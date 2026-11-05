# Auto-boot Station / Monitor: secuencia verificada

## Circuito confirmado por esquema

Dos NPN SS8050 cruzados: T2 tiene base en DTR (por R24) y emisor en RTS;
su colector va a MCLR. T1 tiene base en RTS (por R23), emisor en DTR y
colector en BOOT. Las salidas necesitan sus polarizaciones de placa.
DTR y RTS NO son controles independientes de RESET y BOOT.

Con CP210x, True en la API corresponde a nivel bajo de la señal USB:

| DTR API | RTS API | Resultado del circuito |
| --- | --- | --- |
| False | False | Ambos transistores apagados; MCLR y BOOT liberados |
| False | True | MCLR bajo (RESET), BOOT liberado |
| True | False | MCLR liberado, BOOT bajo |
| True | True | Ambos transistores apagados; MCLR y BOOT liberados |

## Secuencia para Galio Flash Tool

```python
# Preparar antes de abrir: dtr=False, rts=False.
# Sin control automatico RTS/CTS ni DSR/DTR.
ser.dtr = False
ser.rts = False
time.sleep(0.100)
ser.rts = True          # RESET activo
time.sleep(0.200)
ser.dtr = True
ser.rts = False         # BOOT activo; sin espera entre estos dos comandos
time.sleep(0.500)
ser.dtr = False         # BOOT liberado
```

El pulso RESET debe durar 200 ms (la linea correspondiente es
`time.sleep(0.200)`). Las dos escrituras de transición no son atómicas: no
introducir una espera entre ellas. Capturar RX desde antes de liberar RESET,
sin purgar el buffer después. Mantener abierto el mismo puerto para la carga.
La identificación llega antes de que el loader acepte datos: esperar 1200 ms
después de recibir su línea completa antes del primer registro HEX, pues el
firmware ejecuta `delay_ms(1000)` tras el banner. Conservar ACK/XON/XOFF del
cargador existente. El script de diagnóstico no transmite datos.

## Ejecutar la prueba

Desde la raíz del repositorio, con el entorno local preparado:

```powershell
& '.\.venv\Scripts\python.exe' tools\autoboot_probe.py --port COM26 --circuit crossed
```

Para otro equipo: Python 3 y pyserial 3.5. `--dry-run --circuit crossed`
muestra la secuencia sin abrir hardware. El modo `independent` queda disponible
para comparar otros circuitos, pero no corresponde a esta placa.

## Resultado real del 2 de noviembre de 2026

La prueba con circuito cruzado fue exitosa en COM26, 115200 8N1:

- RTS=True a 482.7 ms: RESET activo.
- DTR=True a 683.9 ms y RTS=False a 684.2 ms: transición a BOOT.
- Identificación completa recibida a 694.6 ms:
  `name=Protolink-Station-Monitor,version=BOOT:1.2,date=2-Nov-26,time=21:14:39`.
- DTR=False a 1194.9 ms: BOOT liberado.
- Sin escritura de flash. No se probó transferencia de HEX en esta sesión.

Las pruebas previas de 500 ms y 2 s no recibieron datos porque usaban el
modelo de controles independientes. No demostraban un fallo de hardware.

## Corrección del análisis inicial de los scripts

`G:/Python/script autoboot pic.py` SI usa la secuencia adecuada para este
esquema: (False,True) durante 2 s fuerza RESET; (True,False) durante 200 ms
fuerza BOOT; después libera DTR. Sus comentarios eléctricos son incorrectos,
lo que llevó a interpretar erróneamente la secuencia antes de ver el esquema.

`G:/Python/autoboot_cp2102_pic.py` trata las líneas como independientes.
En este circuito, pasar por (True,True) libera ambos transistores, por lo que
no representa el pulso de RESET que describen sus comentarios.

Ambos scripts originales cierran el puerto al terminar y no confirman el banner.
El diagnóstico nuevo captura el banner, pero tampoco sustituye Galio Flash Tool.
La documentación de [pySerial](https://pyserial.readthedocs.io/en/latest/pyserial_api.html)
advierte de posibles transitorios de DTR/RTS al abrir el puerto.

## Cargador optimizado

`tools/station_upload.py` usa ahora registros de hasta 26 bytes, sin cruzar filas
flash de 128 bytes del PIC18F67K40. Cada línea ocupa como máximo los 64 bytes del
buffer del loader, incluido CR, y cada registro espera XOFF/ACK/XON antes del
siguiente. El script conserva los fuses y la reserva del bootloader.

Secuencia del cargador: reposo 50 ms, RESET 200 ms, BOOT posterior 200 ms.
La recepción del banner se captura durante esa secuencia. El envío empieza al
cumplir 1050 ms desde el banner completo, sin volver a sumar toda la espera.
El script de diagnóstico conserva sus tiempos conservadores configurables.

La consola informa líneas HEX originales, registros normalizados, bytes serie y
tiempos de transferencia/entrada+carga. Ver los resultados en `validation.md`.

## Galio Flash Tool (C#): integración de campo

La aplicación Windows incorpora el perfil Station/Monitor BOOT:1.2 y la secuencia
optimizada 50/200/200 ms, con 1050 ms desde el banner completo. Cada pulsación de
Cargar valida el HEX antes de abrir el puerto y ejecuta de nuevo autoboot.
La carga mantiene el mismo puerto hasta comprobar el arranque y después lo libera.

Se portaron la protección de memoria/fuses, el empaquetado de hasta 26 bytes sin
cruzar filas de 128 bytes y las confirmaciones XOFF/ACK/XON más el ACK/XON final.
El resultado distingue carga aceptada de inicio de aplicación observado; no hay
verificación por lectura de flash. Los registros se guardan en
`%LOCALAPPDATA%\Galio\FlashTool\logs`, incluyendo SHA-256 del archivo.

La implementación C# pasó compilación Release y 23 comprobaciones automatizadas
con transporte simulado, además de una comprobación de construcción y render de
la interfaz. **Pendiente validar una carga física desde Galio Flash Tool**; las
mediciones anteriores de este documento corresponden al cargador Python.

### Validación física de Galio Flash Tool — 2 de noviembre de 2026, 23:17

Se completaron dos cargas consecutivas desde la interfaz Windows en COM26 con
`build/station-dht22/main.hex` (SHA-256
`BE9CD47070FDBC3C84221E18CA924831A968E2DCB2371EB4E8A944BA159677BD`).
8034 bytes de aplicación, 12 bytes de fuses excluidos, 317 registros. Ambas
recibieron el banner BOOT:1.2, las confirmaciones completas y el arranque
`INFO app=station-dht22 fw=0.1.0 ... reset=0x0333`.
Aproximadamente 5.0 s hasta aceptación y 6.0 s hasta identificación de aplicación.

La primera implementación C# con DtrEnable/RtsEnable no entró al loader: arrancaba
la aplicación, y se detenía antes de enviar HEX. Se contrastó con autoboot_probe
(sin escribir flash), que sí funcionó con 50/200/200 ms. Se sustituyó el transporte
de carga por un handle Win32 propio y EscapeCommFunction para las líneas, evitando
la reconfiguración SetCommState de las propiedades de SerialPort en cada cambio.
No se alteraron los tiempos ni el protocolo. Las dos cargas correctas corresponden
a esta versión nativa; esto resuelve la validación física pendiente de carga normal.

Logs locales de Galio en `%LOCALAPPDATA%\Galio\FlashTool\logs`:
- `20261102-231703-133-9fb3b6d254bf4cacacd258d919719ccf.log`
- `20261102-231730-045-43bc2f6ceda54148bebfd1a1920e9abd.log`

COM26 quedó liberado al terminar; la placa ejecuta station-dht22 0.1.0. No se midieron
señales con osciloscopio ni se comprobó funcionalmente el sensor en esta prueba.
