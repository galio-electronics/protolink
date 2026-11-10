# Desarrollo de aplicaciones

Copiar [station-demo](../firmware/apps/station-demo/README.md) para crear una app.
Cada app contiene `config/project.json`, `config/dependencies.json` y su workspace.
Las fuentes se resuelven desde la carpeta de la app. Sus dependencias viven en
`<app>/galio` y los resultados en `<app>/build/<perfil>`.

Los encabezados de placa permanecen en la raíz y los módulos reutilizables en
`firmware/common`. Las apps existentes son demos de sensores e integraciones.

## Agregar drivers

Declarar el repositorio y commit en el manifiesto de la app; consultar la
[guía de dependencias](dependency-setup.md). Configurar macros antes de los includes.
Incluir la implementación una sola vez segun las convenciones del driver.
Revisar UART, ISR, timers, buses, buffers y tiempos límite. UART1 es de diagnóstico.
`station.h` usa fast_io: configurar TRIS además de escribir el pin.
Inicializar en `app_init` y atender en tareas cooperativas sin bloqueos largos.

## Compilar y validar

### Buzzer activo compartido

`firmware/common/buzzer.h` declara la API y `buzzer.c` implementa pulsos
cooperativos. En `app_config.h` definir `BUZZER_PIN` (por ejemplo `BUZZ_OUT`)
y `BUZZER_ACTIVE_LEVEL` (1 para activo en alto, 0 para activo en bajo).
Incluir `../../common/buzzer.c` una sola vez, despues del encabezado de placa.
El modulo requiere `protolink_millis()` con Timer2 inicializado.

Llamar `buzzer_init()` en `app_init()` y `buzzer_task()` frecuentemente desde
el bucle principal (`APP_FAST_TASK=1` y `app_task()` en el ejecutor comun).
`buzzer_beep(ms)` acepta 1..65535 ms; una nueva llamada reinicia la duracion.
`buzzer_beep(0)` y `buzzer_stop()` apagan inmediatamente. Solo llamar desde
main, nunca desde ISR. El apagado tiene la latencia del bucle principal y
el calculo de tiempo tolera el desbordamiento del contador de milisegundos.
No usa PWM ni reserva otro timer; corresponde a buzzers activos.

### ADC compartido: VIN y AUX

`firmware/common/station_adc.h` y `station_adc.c` controlan el ADC de 10 bits
del PIC18F67K40 a 64 MHz. Habilitan AUX_ADC (RA0/AN0) y VIN_ADC (RA1/AN1),
con referencia VSS/VDD; los demas pines quedan digitales. No combinar con
otros drivers ADC. Sustituye al modulo provisional `vin_adc`.

Configurar `STATION_ADC_VREF_MV` con VDD medido (3300 mV nominales).
`STATION_ADC_VIN_R1_KOHM`/`STATION_ADC_VIN_R2_KOHM` valen 100/10;
`STATION_ADC_AUX_R1_KOHM`/`STATION_ADC_AUX_R2_KOHM` valen 10/10.
R1 conecta la entrada con el pin ADC y R2 conecta el pin con GND.
VIN usa `round(raw * 36300 / 1023)` y AUX `round(raw * 6600 / 1023)`.
AUX devuelve el voltaje original del sensor de 0-5 V, no el voltaje dividido.
5 V producen aproximadamente 775 cuentas. La precision depende de VDD y
las tolerancias de las resistencias. Los pines deben permanecer entre VSS
y VDD; el fondo de escala calculado no es el limite de alimentacion de la placa.

Incluir `station_adc.c` una vez despues del encabezado de placa. Llamar
`station_adc_init()` en app_init, `station_adc_start()` para solicitar el par
y `station_adc_task()` desde app_task (`APP_FAST_TASK=1`). Requiere Timer2
inicializado y llamadas desde main, nunca ISR. El modulo adquiere AUX y luego
VIN; espera dos ticks despues de cada cambio de canal sin bloquear y consulta
`adc_done()` antes de leer. start devuelve FALSE si hay una secuencia pendiente.

`station_adc_ready()` solo es verdadero cuando ambas lecturas estan completas;
se invalida al aceptar una nueva solicitud. Consultar entonces
`station_adc_raw(canal)`, `station_adc_millivolts(canal)` y
`station_adc_saturated(canal)`, usando `STATION_ADC_VIN` o `STATION_ADC_AUX`.
Las lecturas son secuenciales, no simultaneas. Saturacion significa 1023 cuentas;
no identifica un sensor desconectado ni una senal AUX que supera 5 V pero sigue
dentro del rango del ADC. Canales invalidos devuelven cero/FALSE.

### Herramientas de compilacion

Abrir el workspace de la app. Las tareas pasan su `--config` a `tools/run.ps1`.
Desde la raíz también se puede usar `--profile` con el nombre de carpeta de la app.
CCS 5.081 recompila la unidad principal con los encabezados comunes y el galio local.
Conservar `compile.log`, listado y `build.json` para diagnóstico.
Los logs serie siguen en `logs/` en la raíz. Cerrar el monitor antes de cargar.

Los ajustes de equipo opcionales en `config/local.json` raíz se aplican primero;
`<app>/config/local.json` tiene prioridad. No versionar credenciales.
Para clientes privados, partir de una copia o fork del repo y mantener comunes,
encabezados y herramientas junto a las apps necesarias.
