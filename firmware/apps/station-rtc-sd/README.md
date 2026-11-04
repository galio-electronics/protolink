# Demo Station: RTC MCP79402 + microSD

Perfil independiente `station-rtc-sd`, CCS PCH 5.081, PIC18F67K40 a 64 MHz.
No sustituye el perfil maestro ni las demos de sensores. Conserva bootloader,
UART1 y scheduler existentes. El arranque monta la SD **sin escribir archivos**;
la escritura se habilita explícitamente con `log on` en consola.

## Compilar y probar

Desde `G:\Proyectos\PIC\protolink`:

```powershell
.\tools\run.ps1 build --profile station-rtc-sd
# Cuando la placa de prueba esté lista:
.\tools\run.ps1 build-upload-monitor --profile station-rtc-sd
```

Se usa el puerto configurado en el proyecto; `--port COMxx` lo reemplaza.
También se puede abrir `station-rtc-sd.code-workspace` y ejecutar sus tareas.
La dependencia fuente está en `G:\Proyectos\PIC\sd-card`; el build ya incluye
`G:\Proyectos\PIC` como raíz de librerías. Carga y prueba física realizadas en
COM26 el 2026-11-03; resultados en `validation.md`.

Consola UART1, 115200 8N1, comandos terminados en Enter:

| Comando | Acción |
| --- | --- |
| `help` | Comandos propios del demo; también siguen `info`, `status`, `inputs`, `errors` |
| `rtc` | Fecha válida o código de error; contador de flancos MFP |
| `rtc raw` | Registros RTC 0x00..0x07 en hexadecimal, sin modificarlos |
| `rtc 20261103193000` | Establecer YYYYMMDDhhmmss, ejemplo; usar la hora real deseada |
| `power` | Timestamps BCD crudos de corte/restauración, sin año, y flag PWRFAIL |
| `power clear` | Reconocer falla: borra PWRFAIL **y sus timestamps** |
| `sd` | Estado SD, capacidad en sectores, estado FAT y errores |
| `sd init` | Reintento manual de inicialización/montaje |
| `sd sector0` | Lee LBA0 y muestra firma; no escribe sectores |
| `log on` | Habilita CSV solo con RTC confiable y FAT montada |
| `log now` | Encola una muestra de entradas digitales, si logging está habilitado |
| `log status` | Cola, escritos, perdidos y mayor duración de escritura observada |
| `log read` | Lee los últimos 64 bytes del último archivo usado en esta ejecución |
| `log off` | Detiene adquisición de registros y vacía lo pendiente si hay SD disponible |

`RTC error`: 1=bus/NACK/timeout, 2=fecha/BCD/formato inválido,
3=oscilador detenido, 4=PWRFAIL pendiente. Se exige revisar `power` y reconocer
la falla antes de considerar confiable la hora, aunque la batería la haya conservado.
`rtc ...` conserva PWRFAIL: ajustar la hora no borra evidencia de cortes.
No se establece la fecha de compilación ni una fecha fija al arrancar.

## Hardware y configuración verificada en fuentes/ensamblador

| Recurso | Configuración |
| --- | --- |
| I2C1 | RC3 SCL, RC4 SDA, dirección RTC 0x6F (0xDE/0xDF en el bus), 100 kHz |
| PPS I2C1 | SCL1IN/OUT=RC3, SDA1IN/OUT=RC4 |
| SPI2 | RD4 MOSI, RD5 MISO, RD6 SCK, RD7 CS activo bajo |
| PPS SPI2 | SDO2=RD4, SDI2=RD5, SCK2IN/OUT=RD6 |
| SPI mode 0 | SSP2STAT=0x40, SSP2CON1=0x2A |
| Inicialización SPI | SSP2ADD=39: 64 MHz/(4*40)=400 kHz |
| Operación SPI | SSP2ADD=15: 1 MHz; configurable con `SD_RUN_HZ` |
| MFP | RG7 entrada, SQW 1 Hz, polling cada 10 ms |
| Digital/TRIS | `protolink_io_init` desactiva ADC/analógicos; TRISC bits 3/4=1; TRISD alto=0x20 |
| Timer2 | Tick de 1 ms existente; no se usa como reloj SPI |

**Hallazgo CCS:** `spi_init(...,400000)` generaba Fosc/64=1 MHz en esta versión.
Se usa MSSP2 en modo BRG explícito con timeout por byte. La revisión del `.lst`
confirma escrituras a SSP2ADD/STAT/CON1 y los divisores 0x27/0x0F.
Las tasas todavía deben medirse con analizador lógico en la placa.

I2C1 usa un adaptador de registros con esperas acotadas porque no existía un
driver compartido con timeouts. Los PPS coinciden con los demos LCD existentes.
No compartir ese bus con otro controlador que lo reconfigure simultáneamente.
Una línea mantenida baja se reporta como falla; no se fuerza recuperación con pulsos.

RTC y SD a 3.3 V; verificar desacoplo, polarización de CS y pull-ups de I2C/MFP.
RG7 no tiene IOC/INTn utilizable aquí; el polling puede perder flancos mientras
una operación SD bloquea. Es diagnóstico, no fuente de tiempo del logger.
La salida SQW no opera durante alimentación exclusiva desde VBAT.

Referencias: [PIC18F67K40 DS40001841G, PPS y MSSP](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/PIC18%28L%29F67K40-Data-Sheet-DS400001841.pdf),
[MCP79400/01/02 DS20005009G](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/MCP79400-MCP79401-MCP79402-Data-Sheet-20005009G.pdf).

## Arquitectura y alcance

1. `firmware/common/mcp79402.c/.h`: calendario 2000–2099, BCD, ST/OSCRUN,
   VBATEN, configuración explícita de hora, diagnóstico de alimentación.
2. `sd-card/sd_card.c/.h`: sectores reales con SDHC/SDSC y timeouts.
3. `sd-card/fat32.c/.h`: un buffer de 512 bytes, montaje FAT32, archivos cortos
   en raíz, anexado, lectura y cierre. Ver límites en el README de esa librería.
4. `firmware/common/station_logger.c/.h`: dos registros estáticos de hasta
   127 bytes, encolado y escritura desde main; no E/S en interrupciones.
5. `main.c`: hooks del scheduler y comandos; `diagnostics.c` añade un hook
   opcional que solo se activa para este perfil.

Cada registro captura su fecha al encolarse; al escribir se elige T/E + YYMMDD.
Se abre, anexa y cierra cada registro. Encabezado solo si el archivo tiene tamaño
cero. Se sincronizan datos/metadatos por registro: no hay temporizador de flush
ni agrupación de varios registros en esta primera versión.

`logger_telemetry(data)` recibe cinco campos ya serializados:
`device_id,signal,value,unit,quality`. `logger_event_code` permite un catálogo
extensible de eventos; `logger_event` utiliza el código genérico `MESSAGE`.
Se rechazan campos de eventos con comillas, comas, controles o caracteres no ASCII,
y telemetría que no tenga cinco campos. No se implementa escape CSV.
El demo registra `DI_RAW`; nombres/dispositivo, intervalo (60 s), tamaños de cola,
nivel y habilitación están en `app_config.h`. IDs del demo: máximo 16 caracteres.
No se guarda configuración persistente ni se implementa Store & Forward.

Ejemplos de formato esperado, **no capturas de una tarjeta física**:

```csv
timestamp,device_id,signal,value,unit,quality
2026-11-03T19:30:00,STATION01,DI_RAW,3,bitmask,GOOD
```

```csv
timestamp,level,source,event,message
2026-11-03T19:30:00,INFO,SYSTEM,LOG_START,Demo logging enabled
```

Nombres `T261103.CSV` y `E261103.CSV`, finales CRLF. Los errores de RTC/SD
se consultan por consola aunque el medio o la fecha impidan crear eventos.

## Fallas y límites

- Reintento de SD/montaje cada 10 s cuando no está disponible; no cada vuelta.
- Primer error de escritura: detener nuevas escrituras, contabilizar y descartar
  cola pendiente. No repetir automáticamente un registro de resultado incierto.
- Cola llena/fecha inválida: registro rechazado y contador de pérdida aumentado.
- La SD puede bloquear main durante sus timeouts; la cola no hace asíncrona la
  escritura. Medir `max_ms`; no se garantiza un período duro de adquisición.
- No format, borrado, prueba destructiva de sectores, FAT16/exFAT, LFN ni
  extensión automática de directorio raíz. Se rechaza raíz llena.
- Las búsquedas FAT están limitadas a 2 s más operaciones en curso. Tarjetas
  llenas/grandes o archivos muy fragmentados pueden producir `FAT_LIMIT`.
- No journaling ni reparación automática. Un corte puede dejar líneas parciales,
  clusters huérfanos o FAT divergentes; revisar la tarjeta en PC antes de reanudar.
- No detección física de tarjeta: el estado listo solo cambia al intentar E/S.
- No se modificaron Monitor/Tesla, librerías externas existentes ni proyecto MPLAB.

## Validación

Compilación CCS 5.081 verificada con cero errores y cero advertencias. Los números
finales están en `build/station-rtc-sd/main.lst` y `validation.md` de este demo.
Los tests host ejecutan los mismos C con tipos CCS traducidos a enteros de ancho
fijo y buses simulados **solo en tests**. No validan PPS, oscilador, batería,
temporización eléctrica, fiabilidad de una SD real ni comportamiento del compilador PIC.

```powershell
.\.venv\Scripts\python.exe tests\rtc_sd\run_host_tests.py --cc ruta\a\tcc.exe
.\tools\run.ps1 tests
```

Prueba física propuesta con una tarjeta FAT32 de ensayo:

1. Arrancar sin SD y sin RTC: comprobar consola, LED y retornos de error.
2. Conectar RTC, ajustar hora explícitamente, leerla durante un minuto; comprobar
   MFP a 1 Hz con pull-up. Revisar `power` antes de ejecutar `power clear`.
3. Insertar SD, `sd init`, `sd sector0`; medir SCK a 400 kHz durante inicio y
   1 MHz después, CS alto fuera de transacciones y dirección I2C 0x6F.
4. `log on`, `log now`, `log status`, `log read`. `log off` y esperar `pending=0`
   antes de retirar; leer los CSV en PC y comprobar fecha, cabecera única y CRLF.
5. Reiniciar, habilitar de nuevo y verificar que anexa sin truncar. Probar fecha
   23:59:59 y confirmar archivos nuevos al cambiar de día.
6. Solo con datos descartables: probar tarjeta llena, extracción y cortes durante
   escritura. Confirmar error/contador, revisar FAT en PC y medir peor latencia.
7. Cortar VCC con batería y restaurar: verificar hora conservada, flag y timestamps;
   SQW puede parar en VBAT. No asumir que PWRFAIL implica pérdida de hora.

Se comprobó físicamente ajuste/lectura del RTC, flancos MFP por polling,
montaje de SD, escritura/lectura CSV y reapertura tras RESET. Quedan pendientes
las pruebas de batería/corte de alimentación, fallas inducidas y mediciones con
analizador lógico. Ver `validation.md` para evidencia y límites.

Para repetir consultas con log y verificar primero la identidad del firmware:

```powershell
.\.venv\Scripts\python.exe tools\rtc_sd_probe.py --port COM26 --command rtc --command sd
```

El probe admite `--command` repetido y `--reset` (primero confirma logger apagado
y cola vacía). `--command @set-clock` ajusta explícitamente hora local UTC-06:00.
