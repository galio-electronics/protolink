# Station Qidian QDY30A: RS485 y Ethernet

Abrir `station-qdy30a.code-workspace` en VS Code. Ctrl+Shift+B compila este
perfil; **QDY30A: Compilar, subir y monitorear** carga la aplicacion por USB.
El workspace principal tambien incluye el perfil `station-qdy30a` en su selector.

```powershell
.\tools\run.ps1 build --profile station-qdy30a
.\tools\run.ps1 build-upload-monitor --profile station-qdy30a
```

## Conexion

- Sensor Qidian QDY30A/QDY30A-B RS485; usuario confirma direccion **1**, **9600 8N1**.
- RS485 de Station: UART4, RC0 TX / RC1 RX. DE=RH0 activo alto;
  /RE=RH1 activo bajo. Conectar A/B y referencia del sensor al puerto RS485.
- Alimentar el sensor segun su etiqueta; no se alimenta desde TX/RX del PIC.
- Ethernet: UART3, RE0 TX / RE1 RX, **115200 8N1**, como la demo DHT22.
- Consola USB: UART1 / COM26 a 115200. Timer2 conserva el tick de 1 ms.
- LED1: latido. LED2: encendido si la ultima lectura fue valida; apagado ante error.
  RS485_STATUS (RC2): pulso de 200 ms por lectura valida, apagado ante error,
  desde firmware 0.1.1. Polaridad configurable en `RS485_STATUS_ACTIVE_HIGH` (1).
  Las salidas DO quedan inactivas. Esta demo no inicializa LoRa ni DHT22.

## Lectura y JSON

Cada cinco segundos consulta **FC03, direccion inicial 0, cantidad 7** mediante
`smodbus_read_holding` de la libreria externa `../simple_master`.
No escribe registros ni cambia direccion, baudrate, cero o calibracion.

| Registro (direccion base cero) | Interpretacion usada |
|---|---|
| 0 | Direccion del sensor |
| 1 | Codigo de baudrate (se muestra en consola) |
| 2 | Codigo de unidad |
| 3 | Decimales, 0..3 |
| 4 | Medicion int16 con signo |
| 5 | Cero del rango, int16 con signo |
| 6 | Fin del rango, int16 con signo |

El valor se calcula como `raw / 10^decimals`, con formato decimal exacto.
La unidad se publica como **unit_code**: existen variantes con tablas diferentes;
no se etiqueta una lectura como metros o centimetros sin confirmar el codigo
con la documentacion del sensor instalado. Esta demo no convierte presion a nivel.
Mapa contrastado con una [implementacion publica del QDY30A-B](https://github.com/vingarzan/qdy30a-b/blob/main/sensor.go);
la placa confirmo respuestas validas con codigo de unidad 17 y un decimal.

Ejemplo ilustrativo de telemetria (no es una lectura de la placa):

```json
{"sensor_ok":true,"value":12.34,"unit_code":1,"raw":1234,"decimals":2,"range_min":0.00,"range_max":100.00,"sample":1}
```

En error se omite la medicion para evitar publicar ceros o datos viejos:

```json
{"sensor_ok":false,"modbus_error":1,"sample":2,"errors":1}
```

Codigos: 1 timeout/trama incompleta o desbordada, 2 CRC, 3 trama incorrecta,
4 excepcion Modbus, 5 direccion informada o cantidad de decimales invalida.
`sample` y `errors` son contadores desde el arranque del PIC.

El PIC emite un objeto JSON completo seguido de **CRLF** por UART3, usando una
cola TX por interrupcion. El modulo Ethernet debe delimitar por CRLF o por un
timeout de empaquetado adecuado y publicar el mensaje completo. Si trabaja
como puente MQTT, usar el topico ThingsBoard `v1/devices/me/telemetry` y las
credenciales del device. El PIC no implementa MQTT, conexion ni autenticacion.
No hace falta decoder: los campos ya son JSON numerico/booleano valido.
El servidor asigna la fecha de recepcion; `sensor_ok` permite alarmar por fallos.
Referencia: [telemetria MQTT de ThingsBoard](https://thingsboard.io/docs/reference/mqtt-api/telemetry/).

## Ajustes y dependencia

Direccion, baudrate, intervalo y UART Ethernet estan en `app_config.h`.
Se corrigio la libreria compartida `simple_master` para:

- Permitir `SMODBUS_STOP_BITS=1` (default 2 conservado para otras apps).
- Esperar el ultimo bit antes de bajar DE. Esta app usa TX4STA.TRMT.
- Acotar la recepcion completa a un timeout, incluso con ruido continuo.
- Evitar retornar una longitud de 256 en un contador de ocho bits.

El parche reproducible esta en `docs/patches/simple-master-station-rs485.patch`.
En una copia limpia de la version original de simple_master, aplicarlo desde
ese repositorio con `git apply ../protolink/docs/patches/simple-master-station-rs485.patch`.
No reaplicarlo sobre esta instalacion: ya esta corregida.
Las nuevas huellas estan registradas en `config/dependencies.json`.

La API Modbus es bloqueante hasta aproximadamente 300 ms mas el envio. Durante
la espera las interrupciones de consola, Ethernet y Timer2 siguen activas, pero
el scheduler cooperativo se retrasa. Esta demo consulta un solo sensor y siete
registros; no pretende validar todas las funciones del driver compartido.

## Validacion

CCS PCH 5.081: **0 errores / 0 advertencias**, ROM 7%, RAM 19-29%.
HEX de aplicacion verificado, reserva del bootloader conservada. Listado CCS
confirma espera de TX4STA.TRMT antes de deshabilitar el transmisor.
Carga fisica completada en COM26 el 3 de noviembre de 2026: 347 registros,
3.980 s de transferencia y 5.429 s de entrada+carga. Arranque 0.1.0 confirmado.
Cuatro lecturas consecutivas validas cada cinco segundos: value=29.9, raw=299,
decimals=1, unit_code=17, baud_code=3. Sin descartes TX/RX en consola.
Log: `logs/20261103-095643-970114-station-qdy30a.log`.
El firmware genera y encola el JSON en UART3 despues de cada lectura.
Pendientes: confirmar la unidad fisica del codigo 17 con el manual del sensor
y verificar recepcion del JSON en el modulo Ethernet y ThingsBoard.

## Driver de sensor reutilizable

La lectura, validacion y conversion QDY30A ahora provienen de
[galio-sensors](https://github.com/galio-electronics/galio-sensors),
fijado por commit y huellas en `config/dependencies.json`. El adaptador usa
`simple_master`; la app conserva configuracion UART/DE/RE, direccion,
intervalo, eventos y telemetria. Ver [guia de integracion](../../../docs/sensors.md).
