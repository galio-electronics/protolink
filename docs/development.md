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
