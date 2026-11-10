# Station RAK3172: demo OTAA

Abrir `station-rak3172.code-workspace` en VS Code. Las tareas **RAK3172:**
fijan el perfil `station-rak3172` independientemente del perfil principal.
Ctrl+Shift+B compila; **RAK3172: Compilar, subir y monitorear** carga por COM26.
Cerrar cualquier otro monitor antes de cargar.

## Hardware y red

Desde firmware 0.2.4, el buzzer activo conectado a BUZZ_OUT (RA5) emite
un pitido de 150 ms al terminar la inicializacion de la app. Se configura
RA5 como salida y se apaga desde la tarea cooperativa, sin esperas bloqueantes
ni PWM. `DEMO_STARTUP_BEEP_MS` ajusta la duracion; `BUZZER_ACTIVE_LEVEL`
vale 1 (activacion en alto). Si el circuito de la placa invierte la senal,
configurar ese nivel en 0. La prueba audible requiere cargar el firmware.

En 0.2.5 se utiliza el modulo reutilizable `firmware/common/buzzer.c`.
Un comando AT sin respuesta durante mas de 3 segundos genera un pitido de
500 ms (`DEMO_LORA_FAULT_BEEP_MS`), incluso si el modulo no responde al
arrancar. Se repite en cada reintento fallido (60 s de espera entre intentos).
JOIN_FAILED y EVENT_TIMEOUT no disparan el buzzer: no prueban perdida de UART.
La deteccion depende del siguiente comando; no hay sondeo continuo durante
el intervalo de 60 segundos entre uplinks.

Validacion 0.2.5 (2026-11-05): CCS 5.081 sin errores, advertencia preexistente
`ctx` en RUI3, HEX validado y carga aceptada en COM26. Arranque 0.2.5,
RAK_DETECTADO, JOINED, TX_DONE y DHT22 confirmados; OLED reporto presente=0.
Las 36 pruebas de herramientas pasaron. Log local:
`logs/20261105-220337-322410-station-rak3172.log`.
Pendiente comprobar sonido y desconexion fisica del modulo para validar
el aviso audible; la sesion de prueba mantuvo comunicacion con el RAK.

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

### Credenciales despues de clonar

En un clon nuevo ejecutar desde la raiz, antes de compilar:

```powershell
python tools/rak3172_credentials.py
```

El generador crea `credentials.local.h` y `registration.local.json` con valores
aleatorios coincidentes; no sobrescribe credenciales existentes.

Para configurar manualmente llaves existentes, se incluyen las plantillas
`credentials.example.h` y `registration.example.json`. Desde esta carpeta:

```powershell
Copy-Item credentials.example.h credentials.local.h
Copy-Item registration.example.json registration.local.json
```

Usar estas copias solo si los archivos locales aun no existen. Reemplazar los
valores ficticios en ambos archivos: `DEMO_JOIN_EUI` / `join_eui` debe contener
16 caracteres hexadecimales y `DEMO_APP_KEY` / `app_key`, 32, sin espacios ni
prefijo `0x`. No usar la clave de ceros del ejemplo en un dispositivo real.

En `registration.local.json`, sustituir `READ_FROM_LORA_QUERY_2` por el DevEUI
de 16 caracteres que muestra el modulo en `LORA query=2 AT+DEVEUI=...`.
Este JSON es una ficha para copiar los valores a los campos de ChirpStack
descritos arriba; no es un cuerpo de importacion de su API. Para el codec de
ChirpStack pegar el contenido de `codec.js`, no este JSON.

Versionar solo los archivos `.example.*`; los `.local.*` contienen las llaves
reales y permanecen excluidos de Git.

### Cambio de modulo RAK

La demo consulta `AT+DEVEUI=?` y conserva el DevEUI de fabrica de cada modulo.
Al reemplazarlo, copiar el identificador de la linea
`LORA query=2 AT+DEVEUI=...` y darlo de alta en ChirpStack. El mismo firmware
configura el JoinEUI y AppKey de `credentials.local.h` en el nuevo modulo.
El DevEUI de `registration.local.json` es un registro local del primer modulo;
no se actualiza automaticamente ni se utiliza para programar la identidad.
`RAK_DETECTADO` confirma UART; solo `JOINED` confirma la activacion LoRaWAN.

## Payload y consola

### VIN y AUX (firmware 0.2.7)

El modulo comun `station_adc.c` coordina ambos canales con ADC de 10 bits
y referencia VDD de 3300 mV (`STATION_ADC_VREF_MV`, ajustar con multimetro):

- VIN_ADC, RA1/AN1: R1=100 kohm y R2=10 kohm; factor 11.
- AUX_ADC, RA0/AN0: R1=10 kohm y R2=10 kohm; factor 2, para sensores de 0-5 V.

R1 conecta la senal al pin ADC; R2 conecta el pin a GND. El voltaje reportado
corresponde a la entrada antes del divisor. No se convierte a unidades del sensor.
VIN representa la alimentacion protegida despues del diodo RB050LAM-60TF,
sin compensar su caida. El usuario confirmo la conversion de VIN y midio
15.08 V en la fuente y 14.67 V despues del diodo; se conserva esta ultima
referencia como voltaje de alimentacion del sistema.
La app solicita un par aproximadamente cada segundo y publica el resultado
anterior (puede posponerlo durante DHT22). Ejemplos de consola:

```text
VIN raw=338 mv=11994 saturated=0
AUX raw=775 mv=5000 saturated=0
```

Las primeras lineas aparecen aproximadamente a los dos segundos. `saturated=1`
indica fondo de escala del ADC, no un voltaje fiable fuera de rango. AUX mayor
que 5000 mV queda visible como tal; no se recorta a 5 V. Las lecturas no se
agregan al payload LoRaWAN ni a la OLED.

Validacion 0.2.7: CCS 5.081 sin errores, advertencia preexistente `ctx` en RUI3,
HEX validado. VIN confirmado por el usuario; pendiente validacion de AUX
con multimetro. Compilacion y HEX revalidados el 2026-11-10.

### OLED (firmware 0.2.3)

OLED SSD1306 de 128x64, direccion I2C de 7 bits `0x3C`, conectada a
I2C1: SCL=RC3 y SDA=RC4, con tierra comun y pull-ups adecuados al modulo.
El bus funciona a 400 kHz. Se reutiliza la libreria externa `oled_i2c`.

La pantalla muestra `RAK3172 / DHT22`, temperatura en Celsius y humedad
relativa en porcentaje, ambas con un decimal (incluidas temperaturas negativas).
Se actualiza con cada lectura del DHT22; ante error muestra guiones y el codigo
del sensor. Al inicio o tras 15 segundos sin dato reciente muestra guiones y
`Sin dato reciente`, evitando presentar mediciones antiguas como actuales.

La app comprueba presencia al arrancar y reporta `OLED ... presente=0/1` en
consola. Si no responde, continua sin pantalla. Conectar antes de reiniciar.
El refresco escribe un caracter cada 10 ms y se pausa durante la captura DHT22.
No cambia el intervalo de muestreo ni el payload LoRaWAN.

Validacion 0.2.3: compilacion CCS 5.081 sin errores, una advertencia preexistente
(`ctx` en RUI3), ROM 11%, RAM 34-37%; HEX validado para el bootloader.
Cargado por COM26: arranque en 0.2.3, OLED detectada, DHT22 a 26.0 C y
45.1-45.2 %RH, JOINED y TX_DONE confirmados por consola. Log local:
`logs/20261103-200908-200249-station-rak3172.log`.
Pendiente comprobar visualmente el contenido en la OLED fisica.

Desde 0.2.2, si el ping AT no recibe OK en tres segundos, la consola avisa
`WARN RAK_NO_DETECTADO` e indica revisar alimentacion, socket, TX/RX de UART5,
115200 baud y modo AT. Esto indica falta de respuesta, no confirma ausencia
fisica: un baudrate o firmware distinto tambien puede causarlo. Reintenta cada
60 segundos. `INFO RAK_DETECTADO UART5: AT OK` confirma la comunicacion.
Si un comando posterior vence, muestra `RAK_SIN_RESPUESTA`, apaga el estado de
union y vuelve a comprobar AT antes de configurar. LED3 vuelve a parpadear.
`JOIN_FAILED` sigue siendo un fallo de union a la red, separado de este aviso.

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
