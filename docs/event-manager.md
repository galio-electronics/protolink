# Hardware Event Manager MVP — 2026-11-04

## Alcance y evidencia de integración

Motor común en `firmware/common/event_*`, integrado en Station master,
RTC/SD, RAK3172 y QDY30A. Compilado con CCS PCH 5.081, probado en host.
No se cargó firmware ni se efectuaron pruebas físicas en esta entrega.
El cambio previo del usuario en `docs/autoboot.md` se conservó.

Inspección antes de implementación: Station usa `station.h`, `app_config.h`
y el ejecutor `station_app.c`. Timer2 genera 1 ms; UART1 y sus ISR pertenecen
a diagnóstico. Las aplicaciones inicializan sus periféricos desde `app_init`.
RAK usa UART5 y parser cooperativo sobre `rui3_at`; QDY30A usa UART4 y
`simple_master`, con transacciones bloqueantes existentes. RTC usa MSSP1,
microSD usa MSSP2 y los drivers externos `galio/sd-card`. No se altera el
bootloader, watchdog, pinout ni proyecto MPLAB hello-world.

No existe aplicación ni encabezado específico Monitor en este repositorio.
`event_variant_monitor.h` es una configuración conservadora de biblioteca,
con todos los periféricos deshabilitados. `tests/events/monitor_compile.c`
comprueba el dialecto CCS, **no es firmware para cargar en Monitor**.

No había uso de EEPROM externa en las fuentes activas. El usuario confirmó
A0/A1/A2 a GND, alimentación 3.3 V y memoria sin datos. Se usa dirección
7 bits 0x50 (0xA0 escritura / 0xA1 lectura). Quedan sin verificar el sufijo
exacto del componente, WP y resistencias pull-up. No se asignó GPIO a WP.
Una escritura protegida se detecta por comparación de lectura posterior.
El adaptador implementa el contrato AT24C512 de la solicitud: 64 KiB,
página de 128 bytes, direccionamiento interno de 16 bits.
La [hoja AT24C512C de Microchip](https://ww1.microchip.com/downloads/en/DeviceDoc/20006161B.pdf)
describe páginas de 128 bytes y ciclo de escritura máximo de 5 ms;
el timeout del motor es 10 ms. Confirmar el sufijo instalado antes de validar
esos límites como garantía eléctrica de producción.

`station_i2c.c` extrae y generaliza el puerto RTC existente; no duplica el
driver RTC. Cada transacción termina antes de devolver control a main.
No hay accesos desde ISR ni simultaneidad entre RTC y EEPROM. RAK/OLED
comparte las mismas asignaciones PPS; se eliminaron sus declaraciones
duplicadas. El puerto común configura 100 kHz. RG7 continúa siendo RTC_MFP;
no se usa el antiguo EE_WP de V2. No se agregaron sondas periódicas a otros
periféricos. La supervisión de alimentación está deshabilitada: no hay divisor
ni límites verificados. El ADS1115 externo de `galio/analog_drivers` entrega
datos sin propagar ACK/error de lectura; se ofrece `event_health_adc`, pero
no se activa ni se presenta esa lectura como comprobación de salud.

## Configuración y flujo

| Aplicación | Máscara de módulos | EEPROM | Supervisión activa |
|---|---:|---:|---|
| station-rtc-sd | 0x0017 | Sí | Sistema, RTC, EEPROM, SD |
| station-rak3172 | 0x000D | Sí | Sistema, EEPROM, respuesta AT/timeout UART |
| station-qdy30a | 0x0045 | Sí | Sistema, EEPROM, transacción QDY30A |
| Monitor, configuración de biblioteca | 0x0001 | No | Solo sistema |

El resto de aplicaciones no habilita el motor. `APP_EVENT_MANAGER` agrega
captura de reset al inicio de main, inicialización después de Timer2 y
diagnóstico, y una llamada a `event_manager_task` por vuelta. `event_station.c`
se incluye antes del código de aplicación. Solo `event_storage_task` toca
EEPROM: un reporte actualiza RAM y la cola, nunca ejecuta una operación física.
No hay malloc ni RTOS. El logger FAT/CSV anterior permanece separado;
no se añadió datalogger de telemetría ni se habilita automáticamente.

API pública: `event_manager.h`, `event_storage.h`, `event_crc.h`,
`event_codes.h`, `event_config.h`. `event_manager_report` usa conexión 0;
`event_manager_observe` permite indicar conexión y fallo SD terminal.
`event_manager_emit` permite eventos excepcionales. Report/observe devuelve
0 si el módulo/observación es inválido o se pierde el evento al saturarse;
1 también puede significar observación aceptada sin transición.
`event_manager_thresholds(module,fail,good)` ajusta umbrales por módulo.
`event_manager_read(index)` inicia una lectura asíncrona, índice cero = más
antiguo; resultado en `event_storage_read_done/read_ok/read_data[32]`.
No conservar el resultado a través de una nueva petición de lectura.
`event_manager_count` cuenta slots válidos conocidos; durante scan es parcial.
`is_busy` indica cualquier estado distinto de READY o una cola no vacía,
incluyendo ERROR y DISABLED. Estos últimos se distinguen por STATUS.

Estados de salud: UNKNOWN=0, OK=1, DEGRADED=2, FAULT=3, DISABLED=4.
Tres observaciones consecutivas confirman fallo/degradación y dos éxito.
Cambiar candidato reinicia el contador; UNKNOWN→OK no genera recuperación.
Los fallos terminales de SD son inmediatos; recuperación requiere dos éxitos.
La salud global refleja el peor módulo habilitado; si solo hay OK/UNKNOWN,
retorna UNKNOWN. `EVENT_RS485_DEVICES` vale 1 en producción y admite 1..8;
cada conexión extra consume 3 bytes de estado, no un buffer de transacción.
Los estados de conexión son independientes y HEALTH publica su agregado.
La cola tiene cuatro registros (128 bytes), más uno ya entregado al storage.
Con saturación, un evento de mayor severidad sustituye al más antiguo de
menor severidad; si no existe uno menor, se rechaza el nuevo. Los retenidos
mantienen orden FIFO. El registro en vuelo nunca se sustituye. Cada pérdida
incrementa `event_queue_lost`; el estado de salud sí cambia aunque se pierda
su evento. No se reintenta indefinidamente el reporte de una transición perdida.

## Catálogo estable

Module IDs independientes: SYSTEM=0, RTC=1, EEPROM=2, LORA=3, SD=4,
ADC=5, RS485=6, POWER=7. La máscara usa bit = Module ID.

| Event ID | Nombre | param1 | param2 |
|---|---|---|---|
| 0x0101 | SYSTEM_BOOT | PCON0 crudo | restart_cause CCS crudo |
| 0x0102 | RESET_WATCHDOG | PCON0 | causa CCS |
| 0x0103 | RESET_BROWNOUT | PCON0 | causa CCS |
| 0x0104 | RESET_EXTERNAL | PCON0 | causa CCS |
| 0x0105 | RESET_SOFTWARE | PCON0 | causa CCS |
| 0x0106 | RESET_STACK_FAULT | PCON0 | causa CCS |
| 0x0107 | RESET_UNKNOWN | PCON0 | causa CCS |
| 0x0201 | RTC_FAULT | rtc_error | sincronizado (0/1) |
| 0x0202 | RTC_RECOVERED | rtc_error | sincronizado (0/1) |
| 0x0203 | RTC_TIME_INVALID | rtc_error | sincronizado (0/1) |
| 0x0301 | EEPROM_FAULT | contador de errores | 0 |
| 0x0302 | EEPROM_RECOVERED | contador de errores | 0 |
| 0x0303 | EEPROM_CRC_ERROR | registros no vacíos inválidos en scan | 0 |
| 0x0401 | LORA_FAULT | 1=timeout UART | 0 |
| 0x0402 | LORA_RECOVERED | 0=OK AT | 0 |
| 0x0501 | SD_FAULT | sd_error | sector; 0xFFFFFFFF para init/sync |
| 0x0502 | SD_RECOVERED | sd_error | sector; 0xFFFFFFFF para init/sync |
| 0x0601 | ADC_FAULT | error del driver | canal (adaptador, no activo) |
| 0x0602 | ADC_RECOVERED | error del driver | canal |
| 0x0701 | RS485_COMM_LOST | resultado de transacción | conexión<<8 OR esclavo |
| 0x0702 | RS485_COMM_RECOVERED | resultado de transacción | conexión<<8 OR esclavo |
| 0x0801 | POWER_LOW | reservado para mV medidos | reservado para umbral mV |
| 0x0802 | POWER_HIGH | reservado para mV medidos | reservado para umbral mV |
| 0x0803 | POWER_RECOVERED | reservado para mV medidos | reservado para umbral mV |

No hay eventos activos de alimentación. Su futura integración necesita
calibración y umbrales distintos de entrada/salida (histéresis).
LoRa JOIN_FAILED o timeout de evento de red no implican fallo de UART/módulo
y no se convierten en LORA_FAULT. RS485 es pérdida de comunicación del
esclavo indicado, no diagnóstico físico del transceptor.

## Formato persistente v1

Serialización explícita, independiente de padding y sizeof; todos los
enteros multibyte son little-endian. Reserved siempre cero.

| Offset | Bytes | Campo |
|---|---:|---|
| 00 | 4 | sequence |
| 04 | 4 | timestamp Unix UTC, cero si no confiable |
| 08 | 4 | uptime_seconds desde inicialización del motor |
| 12 | 2 | event_id |
| 14 | 1 | module_id |
| 15 | 1 | previous_state |
| 16 | 1 | new_state |
| 17 | 1 | severity_flags |
| 18 | 4 | param1 |
| 22 | 4 | param2 |
| 26 | 2 | boot_id |
| 28 | 2 | reserved |
| 30 | 2 | crc16 |

severity_flags: bits 1:0 INFO=0, WARNING=1, ERROR=2, CRITICAL=3;
bit 2 TIME_INVALID; bit 3 EXTERNAL (RS485); bit 4 RESET_LATE
(captura al entrar en aplicación); bits 7:5 reservados cero.
CRC-16/CCITT-FALSE sobre bytes 0..29: polinomio 0x1021, init 0xFFFF,
sin reflexión, XOR final cero. Vector ASCII `123456789` → 0x29B1.
CRC se guarda little-endian. SD/ADC/LoRa/RS485 fault = ERROR, RTC
tiempo inválido = WARNING, recuperaciones = INFO; reset = WARNING,
boot = INFO. La API excepcional admite CRITICAL.

## Mapa, recuperación y límites

| Región | Uso definitivo del motor |
|---|---|
| 0x0000–0x0FFF | Reservada, no accedida |
| 0x1000–0x1FFF | Reservada, no accedida |
| 0x2000–0x2FFF | Reservada, no accedida |
| 0x3000–0x3FFF | Reservada, no accedida |
| 0x4000–0xFFFF | 1536 registros de 32 bytes, ring |

Dirección = 0x4000 + slot*32, slot 0..1535. El puerto rechaza direcciones
inferiores o no alineadas. Un registro nunca cruza una página de 128 bytes.
No se escriben punteros ni cabeceras fijas; no hay borrado durante init.
El scan lee un slot por llamada, conserva un bitmap de 192 bytes y el
registro con secuencia más reciente. Admite eventos nuevos en cola durante
scan. `scan_ms` mide tiempo de software desde inicio hasta la última llamada
de scan (no es todavía una medición física de latencia validada).

Comparación modular: a es posterior a b si 0 < uint32(a-b) < 2^31.
El historial válido debe abarcar menos de 2^31 secuencias y proceder de
este escritor circular. Lectura cronológica recorre físicamente desde el
próximo slot, saltando huecos; no reordena registros arbitrariamente
importados. CRC corrupto se ignora y se informa una vez por scan mediante
EEPROM_CRC_ERROR. CRC16 detecta corrupción, no garantiza atomicidad ni
detecta todas las combinaciones posibles: una colisión puede aceptar un
registro roto; un registro falso con CRC válido puede desorientar recuperación.
Al sobrescribir, un corte puede perder el registro antiguo y el nuevo del
slot, no se promete conservar ambos. El bitmap se invalida antes de escribir.

sequence inicia en 0; boot_id inicia en 1 y se obtiene como último boot_id+1
(modulo 65536). No es un contador infalible de todos los arranques: si un
arranque no logra persistir nada, el siguiente puede repetir boot_id.
Actualizar firmware conserva historia mientras no se cambie formato/mapa;
no hay rutina de borrado en arranque. El ensayo host simula esa reinicialización,
no una programación ICSP real ni la configuración de un programador externo.

Estados storage: INIT=0 (reservado), SCANNING=1, READY=2, PREPARE=3,
WRITE=4, WAIT_READY=5, VERIFY=6, ERROR=7, READING=8, DISABLED=9.
Tres intentos con pausa de 20 ms; después ERROR y nueva ronda cada 5 s.
Se conserva el registro en vuelo y se reintenta el mismo slot/secuencia,
incluyendo si la escritura anterior quedó terminada pero faltó verificarla.
Un fallo de scan reintenta ese mismo slot, sin declarar vacío el chip.
EEPROM_FAULT se conserva solo si cabe en RAM; EEPROM_RECOVERED requiere
volver a READY. No se promete guardar un error en una EEPROM inaccesible.
El lector valida nuevamente CRC; un fallo de lectura se devuelve al cliente,
sin lanzar recuperación del hardware. Count es la última membresía verificada,
no un nuevo escaneo en cada consulta.

LOG CLEAR es explícito, aceptado solo en READY: escribe 0xFF incrementalmente
en todos los slots y verifica. No borra regiones ajenas. No es atómico:
un corte deja el sufijo todavía no borrado recuperable. Los eventos recibidos
durante clear pueden llenar la cola y se persisten después; count puede volver
a crecer. Se conserva sequence en RAM tras clear; al reiniciar vacío vuelve
a cero. No usar clear como procedimiento de arranque o de actualización.

## RTC, reset y tiempos de ejecución

RTC verifica BCD/calendario, ST/OSCRUN, PWRFAIL y comunicación mediante el
driver existente. Solo un ajuste exitoso `rtc YYYYMMDDhhmmss` establece
sincronización UTC en RAM; se pierde tras reset/error. PWRFAIL debe reconocerse
explícitamente con el comando existente y volver a ajustar la hora. No se
asume que una fecha plausible persistida equivale a hora sincronizada.
Cache UTC caduca después de 2 s sin lectura exitosa. Reportar eventos nunca
lee el RTC. Uptime acumula diferencias de tick y tolera wrap de millis si
se llama task al menos una vez cada 2^32 ms; resolución de 1 segundo.

PCON0 se captura antes de GPIO, Timer2 y `diagnostics_init`. El listado CCS
verificó lectura de FD7 antes de `restart_cause`, que después escribe 0x3F
y ejecuta CLRWDT. Se reutiliza ese rearme, no se limpia la evidencia antes.
PCON0 0x3C/0x3D se identifica como POR a través de SYSTEM_BOOT sin inventar
un nuevo ID; 0x3E BOR, 0x2F/0x1F watchdog, 0x37 MCLR, 0x3B software,
0xBF/0x7F stack; combinaciones restantes UNKNOWN. El bootloader y startup
preceden esta captura: RESET_LATE avisa que no es captura al primer ciclo de
reset y no demuestra qué causa vio previamente el bootloader.
Referencia de dispositivo: [PIC18F67K40, documentación Microchip](https://www.microchip.com/en-us/product/PIC18F67K40),
capítulo Resets; constantes adicionales comprobadas en `18F67K40.h` de CCS.

Implementación **cooperativa con transacciones I2C síncronas acotadas**,
no completamente asíncrona. El puerto conserva esperas de hasta 1000
iteraciones con delay_us(2), más instrucciones/ISR, por fase MSSP.
A 100 kHz una transferencia normal de 32 bytes tarda aproximadamente
3–4 ms de bus; los límites acumulados bajo clock stretching son mayores
(hasta decenas/cientos de ms sumando todas las fases). No se midieron aún
WCET, jitter ni tiempo real de scan en placa. ACK polling ocurre entre
vueltas, no en un delay de 5 ms. El motor no añade operaciones RTC/SPI
ni toca hardware desde ISR. Persisten bloqueos ajenos: set RTC puede esperar
segundos, SD/FAT y Modbus tienen esperas, QDY conserva su espera TX existente.
No se declara que todo el firmware sea no bloqueante.

## Diagnóstico y uso

UART1, comandos sensibles a mayúsculas para preservar `log ...` del demo:

```text
LOG STATUS
LOG COUNT
LOG READ 10
LOG CLEAR
HEALTH STATUS
```

READ devuelve `LOG ` + 64 dígitos hex + CRLF, bytes originales completos.
Espera espacio en TX para no truncar el registro. STATUS muestra estado,
count, queue, lost, err y scan_ms. Busy/range requiere reintentar después;
no se permite una segunda lectura mientras hay respuesta pendiente.
HEALTH usa IDs/estados numéricos descritos arriba. Texto solo en diagnóstico.

## Validación y recursos

Comandos reproducibles:

```powershell
.\tools\run.ps1 build --profile station-rtc-sd
.\tools\run.ps1 build --profile station-rtc-sd
.\tools\run.ps1 build --profile station-rak3172
.\tools\run.ps1 build --profile station-qdy30a
.\.venv\Scripts\python.exe tests/events/run_host_tests.py
.\.venv\Scripts\python.exe tests/rtc_sd/run_host_tests.py --cc build/host-tools/tcc/tcc.exe
.\tools\run.ps1 tests
```

Host usa el motor C real con tipos stdint; solamente RTC requiere adaptación
de nombres de enteros CCS. `--cc` permite otro compilador C. TinyCC está en
build/host-tools de este entorno, no se versiona. En este equipo requirió
ejecución fuera del sandbox por fallo de carga de DLL (WinError 623).

Pasaron las tres suites de eventos: CRC válido/inválido; endian/offsets;
primer evento; EEPROM vacía sin escrituras; reinicio; CRC corrupto; los 32
prefijos posibles de escritura interrumpida; ring de 1600 inserciones;
wrap de secuencia; cola/sustitución por severidad; scan con eventos pendientes;
RTC/UTC/sincronización/calendario/PWRFAIL/recuperación; LoRa/ADC/SD;
RS485 con conexión independiente y límite de índice; bus desconectado;
WP simulado; fallo compartido RTC/EEPROM; causas reset; clear; región ajena
intacta; módulos Monitor deshabilitados sin acceso a bus ni falsas alarmas.
También pasaron las suites previas FAT/RTC/LOGGER/SD y 19 tests Python.
Son simulaciones, no pruebas de fallos eléctricos, UART real ni EEPROM real.

Comparación de listados CCS reales; base recompilada desde HEAD en
`build/events-baseline`, finales en `build/<perfil>`. Unidades: bytes.

| Perfil | ROM base → final (delta) | RAM main base → final | RAM peor caso base → final (delta) |
|---|---:|---:|---:|
| Station, SD deshabilitada | 3328 → 11674 (+8346) | 294 → 822 | 343 → 876 (+533) |
| Station RTC/SD habilitada | 27784 → 37500 (+9716) | 1195 → 1723 | 1505 → 2092 (+587) |
| Station RAK3172 | 14034 → 22820 (+8786) | 1216 → 1744 | 1318 → 1863 (+545) |
| Station QDY30A | 8930 → 18038 (+9108) | 696 → 1224 | 1047 → 1575 (+528) |

Los cuatro builds terminaron sin errores; RAK conserva el aviso previo
`ctx` no utilizado en la dependencia externa. La prueba de compilación
Monitor pasó sin errores/avisos; no representa el consumo de una aplicación
Monitor. El validador HEX verificó las reservas del bootloader de Station.

## Pendientes de hardware y siguiente versión

Validar en placa ACK/lectura/escritura/WP, sufijo EEPROM, recuperación al cortar
alimentación, GPIO sin alteraciones, jitter de main, scan_ms y fallos compartidos.
Comprobar causas reales POR/BOR/WDT/MCLR/stack a través del bootloader y
retención tras carga real de firmware. No se modificó el bootloader para
transportar evidencias del primer reset. Monitor necesita su board port;
ADS1115 necesita propagación fiable de errores en el driver propietario;
alimentación necesita esquema/divisor e histéresis calibrada.

V2: I2C completamente asíncrono con deadline global por transacción;
presupuesto de scan por tiempo; almacenamiento redundante/commit si se
requiere garantía superior a CRC16; configuración de sincronización UTC
persistente y verificable; telemetría de pérdidas por módulo y conexión;
lector paginado con snapshot; pruebas físicas automatizadas. Mantener el
formato v1 o definir migración explícita antes de cambiar bytes persistentes.
