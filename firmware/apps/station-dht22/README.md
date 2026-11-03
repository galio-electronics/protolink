# Station DHT22 + LCD + Ethernet

Abrir `station-dht22.code-workspace` en VS Code (Archivo > Abrir area de trabajo desde archivo).
Este workspace siempre compila `firmware/apps/station-dht22/main.c` con el perfil
`station-dht22`; no cambia el perfil activo del workspace maestro.

## Uso

- Ctrl+Shift+B: DHT22: Compilar.
- Terminal > Ejecutar tarea > DHT22: Compilar, subir y monitorear.
- DHT22: Monitor serie: consola USB UART1 a 115200, actualmente COM26.
- Terminar el monitor con Ctrl+C antes de volver a subir (el puerto es exclusivo).
- `app_config.h`: intervalo de lectura, baudrate Ethernet y direccion LCD.
- `main.c`: logica de la aplicacion. `ethernet_json.c`: cola de transmision UART3.
- Salida de compilacion: `build/station-dht22/main.hex` desde la raiz del repo.

Desde la raiz del repositorio:

```powershell
.\tools\run.ps1 build --profile station-dht22
.\tools\run.ps1 build-upload-monitor --profile station-dht22
```

## Conexiones y comportamiento

DHT22 DATA en DI_1WIRE (RA2), tierra comun y pull-up externo (aprox. 5.1 kohm)
a una alimentacion compatible con el sensor y el nivel electrico de la placa.
El conector se llama 1WIRE, pero DHT22 usa su propio protocolo, no Dallas OneWire.
LCD existente 16x2 PCF8574 en I2C1 RC3/RC4, direccion de 7 bits 0x27.
Ethernet: UART3 TX RE0 / RX RE1, 115200 baud, 8N1.

Primera lectura cinco segundos despues de inicializar, luego cada cinco segundos.
LCD muestra temperatura en grados C y humedad relativa en porcentaje.
UART3 envia exclusivamente un objeto JSON seguido de CRLF, por ejemplo:

```json
{"temperature":24.6,"humidity":58.2}
```

El modulo debe delimitar el mensaje completo (CRLF o timeout adecuado) y publicar
el objeto como telemetria de ThingsBoard. Conexion, autenticacion, topico y
reintentos de red quedan en el modulo. No se implementa MQTT en el PIC.
No se incluye timestamp: ThingsBoard puede asignarlo al recibir la telemetria.
`uart3_queued=1` en la consola USB significa encolado para UART, NO confirmacion
MQTT ni recepcion en ThingsBoard.

Si falta el sensor o falla una lectura, se muestra error y NO se envia una medida
inventada ni la lectura anterior como nueva. Se reintenta al siguiente intervalo.
Codigos: 2 timeout, 3 checksum, 4 rango, 5 pulsos fuera de tolerancia, 6 bus bajo.
La consola mantiene info/status/inputs/errors. LED1 sigue parpadeando y las ocho
DO permanecen en su estado inactivo alto.

## Recursos compartidos

Reutiliza station.h, bootloader existente, scheduler, diagnosticos y lcd_i2c.
El driver comun dht22.c reserva INT0 mapeado a RA2 y Timer1 a 2 MHz; Timer2
sigue proporcionando el reloj del sistema. Mide intervalos entre flancos de
bajada en interrupciones con tiempos limite y checksum de los cinco bytes.
RA2 no puede mapearse a CCP en este PIC. La latencia de otras interrupciones
puede afectar la medicion: validar lecturas con el hardware y la carga real al
agregar mas drivers. No bloquear interrupciones durante una lectura.
APP_FAST_TASK habilita el hook cooperativo app_task() solamente en esta app.
La actualizacion LCD se reparte por caracteres y se pausa mientras se lee DHT22.
UART3 transmite por interrupcion; no mezcla logs de depuracion con telemetria.

Referencias:
- DHT22/AM2302 Aosong: https://www.aosong.com/uploadfiles/2025/04/20250417105409216.pdf
- PPS Microchip: https://onlinedocs.microchip.com/oxy/GUID-E7E38863-5F8F-4375-8442-CC9D6276A523-en-US-12/GUID-5FAFA497-F614-493F-BF59-D11FE32DB715.html
- JSON ThingsBoard: https://thingsboard.io/docs/reference/mqtt-api/telemetry/

## Validacion

Compilado con CCS PCH 5.081: cero errores y advertencias; 8034 bytes de aplicacion,
507 lineas HEX originales y 317 registros de carga optimizada.
Cargado por autoboot en COM26: transferencia 3.631 s, entrada+carga 5.192 s.
LCD responde ACK (lcd=1); falta confirmacion visual de las medidas en pantalla.
Tras corregir el circuito, se verificaron tres lecturas consecutivas cada 5 s:
27.0 C / 54.1 %, 27.0 C / 54.0 %, 27.0 C / 54.0 %.
Las tres pasaron validacion de pulsos, checksum y rango; uart3_queued=1.
Consola sin descartes TX/RX. La recepcion en el modulo y ThingsBoard sigue
pendiente: el indicador de cola no confirma entrega por red.
Log: logs/20261102-224910-786789-station-dht22.log.
Firmware maestro compila sin errores; 12 pruebas del flujo de carga aprobadas.


