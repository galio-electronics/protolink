# QDY30A por LTE / MQTT

Demo independiente basada en `station-qdy30a` (Ethernet). Usa la misma lectura
FC03 de siete registros y el mismo JSON, ahora publicado por el A7670.

## Conexion confirmada

- A7670 UART5: RE2 TX del PIC hacia RX del modem, RE3 RX desde TX del modem,
  115200 8N1, masa comun y adaptacion electrica apropiada de la placa.
- SIMCOM_KEY = RD2, activo bajo directo. Se libera como entrada tras el pulso
  de 100 ms; no se fuerza nivel alto sobre PWRKEY.
- SIMCOM_SLEEP = RD1, bajo para mantener DTR despierto.
- QDY30A: UART4 RC0/RC1, RS485 DE RH0 y /RE RH1, direccion 1, 9600 8N1.
- Consola UART1 a 115200. UART3 no se usa. No inicializar RAK3172 junto con
  esta demo porque comparte UART5.

Al iniciar espera 15 s y prueba AT antes de pulsar KEY. Si ya responde no pulsa.
Si no responde, pulsa una vez y espera el arranque. La polaridad y los tiempos
estan en `lte_config.local.h`. La secuencia no sustituye una señal STATUS: si no hay
respuesta por cableado/baud incorrectos, no puede distinguirlo de modem apagado.

## Configuracion y compilacion

Copiar `lte_config.example.h` a `lte_config.local.h` (ignorado por Git).
La configuracion real permanece solo en ese archivo local.
Editar **lte_config.local.h**: LTE_APN, LTE_MQTT_HOST (sin tcp://), LTE_MQTT_PORT,
LTE_MQTT_CLIENT_ID unico, LTE_MQTT_USER, LTE_MQTT_PASSWORD y LTE_MQTT_TOPIC.
Los valores de ejemplo no son un servidor operativo. Para ThingsBoard,
usuario = token del dispositivo, password vacio y topic v1/devices/me/telemetry.
Esta version usa MQTT TCP sin TLS y APN sin autenticacion PAP/CHAP.

Dependencia descargada automaticamente: `galio/lte-module` con common/ y simcom/.
Abrir `station-qdy30a-lte.code-workspace` o desde la raiz de protolink:

```powershell
.\tools\run.ps1 build --profile station-qdy30a-lte
.\tools\run.ps1 build-upload-monitor --profile station-qdy30a-lte
```

Cada cinco segundos, si MQTT esta listo, lee el sensor y publica su JSON.
Si la lectura falla publica sensor_ok=false y modbus_error. Durante desconexion
se pausan las lecturas; no hay cola offline. No modifica calibracion ni registros
del sensor. Ver el README de `station-qdy30a` para mapa de registros y unidades.
El puerto UART recibe por interrupcion; Modbus conserva su espera bloqueante
de hasta unos 300 ms. El avance AT se ejecuta en APP_FAST_TASK.

Consola: state=20 listo, 21..25 enviando, 255 requiere reiniciar modem y MCU.
published cuenta publicaciones confirmadas, no solamente JSON encolados.
Errores: 3 rechazo AT/MQTT, 4 timeout, 5 overflow, 10 configuracion,
11 desconexion durante transferencia de datos. Consultar README de lte-module.

## Validacion fisica: 3 de noviembre de 2026

CCS PCH 5.081: 0 errores / 0 advertencias, ROM 14%, RAM 46-56%.
HEX validado y cargado por COM26: 697 registros, transferencia 8.020 s.
Se corrigio CMQTTCONNECT para omitir el parametro password cuando esta vacio:
el modem rechazaba el argumento explicito vacio con ERROR.

SIM lista, registro LTE y servicio MQTT confirmados. Conexion confirmada con
+CMQTTCONNECT: 0,0. Durante 100 s de captura (incluyendo arranque), 14 lecturas
QDY30A y 14 publicaciones QoS 1 confirmadas con +CMQTTPUB: 0,0, cada cinco
segundos. value=29.9, raw=299, decimals=1, unit_code=17. Sin error Modbus;
consola tx_drop=0 y rx_drop=0. Estado final 20, error=0, published=14.

Log: logs/20261103-181049-401398-station-qdy30a-lte.log, relativo a la raiz.
No se inspecciono el dashboard del servidor: se verifico la confirmacion MQTT
reportada por el modem. No se probaron cortes de red ni arranque desde modem
fisicamente apagado. La placa queda ejecutando esta version.

El diagnostico muestra transiciones de estado y resultados MQTT/CME sin
imprimir comandos de conexion ni credenciales.

## Driver de sensor reutilizable

La lectura, validacion y conversion QDY30A ahora provienen de
[galio-sensors](https://github.com/galio-electronics/galio-sensors),
fijado por commit y huellas en `config/dependencies.json`. El adaptador usa
`simple_master`; la app conserva configuracion UART/DE/RE, direccion,
intervalo, eventos y telemetria. Ver [guia de integracion](../../../docs/sensors.md).
