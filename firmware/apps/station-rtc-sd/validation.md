# Validación del demo — 2026-11-03

## Compilación real

CCS PCH 5.081 (13314), perfil `station-rtc-sd`, versión final cargada en COM26:

| Medida | Resultado |
| --- | ---: |
| Errores / advertencias | 0 / 0 |
| ROM | 27 784 bytes (21 %) |
| RAM nivel main | 1 195 bytes (34 %) |
| RAM peor caso | 1 505 bytes (42 %) |
| Pila | 12 de 31 niveles |
| Buffer de sector | 512 bytes, único |
| Cola del logger | 2 registros, texto de 128 bytes por registro |

Fuente de métricas: `build/station-rtc-sd/main.lst`; log y hash HEX registrados
en `compile.log` y `build.json` del mismo directorio. El validador del proyecto
verificó que el HEX de aplicación respeta la reserva del bootloader.
El perfil `station-master` también compiló con 0 errores/advertencias tras añadir
el hook opcional de consola (ROM 3328 bytes, RAM 8–10 %).

En el ensamblador del demo se verificaron SSP2CON1=0x2A, SSP2STAT=0x40 y
SSP2ADD=0x27 en inicialización, luego 0x0F. Esto corrige la selección Fosc/64
que generaba CCS al solicitar 400000 baud mediante `spi_init`.

## Pruebas host ejecutadas

TinyCC portable 0.9.27, sin instalación global. Código C de producción con
adaptación de tipos CCS a `uint8_t/uint16_t/uint32_t/bool`; callback printf
adaptado solo para el logger. Los buses y el disco de pruebas viven en RAM.

- FAT32: MBR y superfloppy; creación, anexado cruzando sectores/clusters;
  cerrar, remontar, reabrir y comparar contenido; dos copias FAT y contadores
  FSInfo; archivos de días diferentes; disco lleno, raíz llena, timeout,
  cadena cíclica, error de lectura/escritura y geometría inválida; conservación
  del marcador de fin del directorio al cruzar sector.
- RTC: fechas/BCD, año bisiesto, día de semana, no alterar fecha al reiniciar,
  reloj detenido, fallo de bus, preservación de PWRFAIL y timestamps,
  reconocimiento explícito y selección SQW.
- Logger: cola llena, rechazo de registros demasiado largos o CSV inválido,
  encabezados, formato de fecha, cambio de archivo a medianoche y detención
  ante escritura fallida.
- SD: secuencia SDHC/SDSC, CRC CMD0/CMD8, OCR/CSD, direccionamiento de
  bloques/bytes, lectura/escritura y CMD13, capacidad, ausencia, inicialización
  ocupada, eco CMD8 incorrecto, token/error de escritura y liberación de CS.

Las cuatro suites terminaron PASS. Los simuladores están en `tests/rtc_sd`,
separados de los fuentes enlazados al PIC. No simulan éxito en firmware.

## Prueba física en COM26 — 19:39 a 19:45, hora de Ciudad de México

CP210x, bootloader Station/Monitor BOOT:1.2. Se cargó el demo y se comprobó
su banner. La carga final: 1088 registros en 12.529 s, confirmación EOF y
arranque observado; no se dispone de lectura de flash para comparación binaria.

La primera prueba reveló problemas que los tests host no podían detectar:

- CCS 5.081 reducía a 8 bits las expresiones ternarias pasadas como argumento
  de 32 bits a `sd_command`: tanto HCS de ACMD41 como direcciones CMD17/CMD24.
  Se sustituyeron por variables de 32 bits y asignaciones condicionales. Se
  verificó en el `.lst` la transferencia de los cuatro bytes a la función.
- La suma `2000 + rtc_decimal(...)` generaba una lectura de registro incorrecta;
  el RTC guardaba 0x26, pero se mostraba año 2000. Se separó conversión y suma.
- El RTC inicialmente detenido necesitó más que la espera original de 200 ms.
  Se amplió la espera acotada de arranque a 2 s y se repitió el ajuste con éxito.
  También se eliminó el ternario de 16 bits al acumular años para el día semanal.

Resultados finales observados:

| Prueba | Resultado |
| --- | --- |
| RTC ajuste explícito | `RTC set=1`, fecha 2026-11-03, hora local actual |
| RTC avance | Lecturas consecutivas avanzan un segundo |
| MFP | Contador de flancos aumenta aproximadamente uno por segundo |
| SD | `ready=1 err=0 sectors=60751872` |
| FAT32 | `ready=1 err=0` |
| Lectura LBA0 | Firma AA55 |
| Primera sesión logger | 3 registros escritos, 0 perdidos, máximo observado 732 ms |
| RESET MCLR 200 ms | Arranque observado, causa 0x0337; RTC conserva la hora |
| Segunda sesión logger | 2 registros escritos, 0 perdidos, máximo observado 51 ms |
| Reapertura/anexado | Lectura de cola del CSV muestra final del registro anterior y nueva línea |
| Estado al terminar | `LOG off pending=0`; puerto cerrado y liberado |

Se escribieron eventos y telemetría de prueba en `E261103.CSV` y `T261103.CSV`.
Ejemplo leído realmente de la tarjeta después del reinicio:

```csv
2026-11-03T19:45:13,STATION01,DI_RAW,0,bitmask,GOOD
```

Evidencia en `logs/` del repositorio:

- `20261103-194340-177105-station-rtc-sd.log`: carga final y arranque.
- `20261103-194352-rtc-sd-probe.log`: RTC y FAT listos.
- `20261103-194412-rtc-sd-probe.log`: escritura y lectura iniciales.
- `20261103-194500-rtc-sd-probe.log`: RESET, conservación de hora, reapertura,
  anexado y apagado del logger.

Las cuatro suites host volvieron a pasar después de estas correcciones.
Los tiempos son muestras observadas, no límites garantizados. No se midieron
señales eléctricas/frecuencia con instrumento, no se cortó VCC ni se retiró
la SD durante escritura, y no se validó respaldo con batería. Tampoco se alteró
la fecha para simular medianoche en esta prueba física (cubierta solo en host).
