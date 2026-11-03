# Station RAK3172: demo OTAA

Abrir `station-rak3172.code-workspace` en VS Code. Las tareas **RAK3172:**
fijan el perfil `station-rak3172` independientemente del perfil principal.
Ctrl+Shift+B compila; **RAK3172: Compilar, subir y monitorear** carga por COM26.
Cerrar cualquier otro monitor antes de cargar.

## Hardware y red

- PIC18F67K40, CCS 5.081, bootloader conservado hasta 0x05FF.
- Socket confirmado por el usuario: UART5, RE2 TX / RE3 RX, 115200 8N1.
- UART1 sigue siendo consola. Timer2 conserva el scheduler y DO1..8 inactivas.
- ChirpStack, US915 (`AT+BAND=5`), FSB1, canales 0..7 (`AT+MASK=0001`).
- OTAA, clase A, ADR habilitado, uplinks sin confirmacion cada 60 segundos.
- DevEUI de fabrica conservado: **AC1F09FFFE24F939**.
- Firmware leido del modulo: **RUI_4.0.6_RAK3172-E**.

## Alta en ChirpStack

1. Crear un perfil US915 con OTAA y clase A; seleccionar la version MAC y
   regional compatible con el firmware RUI instalado (LoRaWAN 1.0.x).
2. Agregar el dispositivo con el DevEUI anterior, en orden hexadecimal normal.
3. Copiar JoinEUI y AppKey de `registration.local.json`. En RUI3 JoinEUI se
   configura mediante APPEUI. Para un perfil LoRaWAN 1.0.x, si ChirpStack llama
   al campo raiz `Network key / NwkKey`, introducir alli esta AppKey.
4. Pegar `codec.js` en el codec JavaScript del perfil.
5. Verificar que el gateway y el servidor habiliten US915 canales 0..7.
   La app reintenta JOIN cada 60 segundos; no hace falta recompilar tras el alta.

La AppKey aleatoria de 128 bits y JoinEUI local se generaron con `secrets` de
Python. `credentials.local.h` y `registration.local.json` estan excluidos de Git;
conservar ambos. El HEX/listado tambien contienen la clave y viven en `build/`,
excluido de Git. La consola no imprime las claves. No se generan claves de sesion
ABP: se derivan durante OTAA. El DevEUI identifica el modulo; no es una clave.

En un clon nuevo ejecutar `python tools/rak3172_credentials.py` desde la raiz
antes de compilar. El generador no sobrescribe credenciales existentes.

## Payload y consola

Desde firmware 0.2.1, LED3 parpadea (500 ms encendido / 500 ms apagado)
durante la configuracion, busqueda y reintentos de red. Queda encendido tras
`JOINED` y vuelve a parpadear si el modem informa `AT_NO_NETWORK_JOINED` o
JOIN fallido. Indica el estado de activacion OTAA; los uplinks sin confirmacion
no permiten detectar inmediatamente una caida del servidor o del gateway.

Firmware 0.2.0: DHT22 en RA2 (DI_1WIRE), driver comun con Timer1 e INT0.
Muestreo cada 5 segundos cuando el modem esta libre; envio cada 60 segundos.
No se envian comandos UART bloqueantes durante la captura del sensor.

FPort 10, payload v2 de 11 bytes, campos multibyte big endian:

| Byte | Contenido |
|---|---|
| 0 | Version: 02 |
| 1 | Bitmap entradas crudas, bit 0=DI1 |
| 2..5 | Contador uint32 (reinicia con el PIC) |
| 6 | Estado DHT: 0 sin dato reciente, 1 OK, 2 timeout, 3 checksum, 4 rango, 5 pulso, 6 bus bajo |
| 7..8 | Temperatura int16 complemento a dos, decimas de Celsius |
| 9..10 | Humedad uint16, decimas de %RH |

Pegar el contenido completo de `codec.js` en **Device profile > Codec > JavaScript**
y guardar. El decoder acepta tambien el payload v1 anterior.
Prueba HEX: `0281000000010100EA022B` = 23.4 C, 55.5 %RH, contador 1, DI1/DI8 activas.
En los datos decodificados aparecen `temperature` y `humidity` como numeros,
ademas de `sensor_ok`, `sensor_status`, `sequence`, `inputs_raw` y `di1`..`di8`.
Para ThingsBoard usar esos campos del objeto decodificado de ChirpStack.
Si el sensor falla o el dato tiene mas de 15 segundos, el decoder omite temperatura
/humedad y publica `sensor_ok=false`; no presenta ceros ni valores viejos como mediciones.
La integracion a ThingsBoard la configura el usuario.

`LORA JOINED` confirma activacion. `LORA uplink` indica solicitud y
`LORA TX_DONE` fin de transmision; no confirma recepcion en el servidor.
Los downlinks se muestran en consola, sin accionar salidas.
`JOIN_FAILED`, `AT_TIMEOUT` y `EVENT_TIMEOUT` indican reintento en 60 segundos.
Los comandos de consola comunes (`info`, `status`, `inputs`, `errors`) siguen activos.

## Revision de la libreria y validacion

Se incluye una sola vez `rui3_at.h` (este incluye su `.c`). El ejemplo antiguo
incluia ambos. La libreria externa no se modifica. Su API bloqueante descarta
eventos y su parser no cubre todas las variantes de JOIN fallido. `main.c`
reutiliza UART/ISR/ring/TX y procesa respuestas sin espera; protege los indices
de 16 bits del ring al extraer bytes en el PIC de 8 bits. Este adaptador depende
de internos del driver: revisar al actualizar `rui3_at` y no mezclar sus APIs
de comando/parser con las de esta app. Lineas mayores de 127 bytes se descartan;
la demo no ofrece recepcion de downlinks largos.

Compilacion: 0 errores, una advertencia preexistente (`ctx` sin usar en el driver),
ROM 8%, RAM 32-34%. HEX de aplicacion validado y cargado en placa. UART5,
version y DevEUI leidos. En firmware 0.2.0 se verificaron DHT22 (25.1 C,
53.7 %RH), JOINED y TX_DONE del primer uplink. Log:
`logs/20261103-092047-834292-station-rak3172.log`. Decoder probado con temperatura
negativa, limite uint32, errores de sensor/rango/longitud y compatibilidad v1.
La recepcion y decodificacion del payload en ChirpStack y ThingsBoard quedan
para validar en el servidor del usuario.

Referencia de comandos:
[Manual oficial RUI3](https://docs.rakwireless.com/product-categories/software-apis-and-libraries/rui3/at-command-manual/).
