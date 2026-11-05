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

Abrir el workspace de la app. Las tareas pasan su `--config` a `tools/run.ps1`.
Desde la raíz también se puede usar `--profile` con el nombre de carpeta de la app.
CCS 5.081 recompila la unidad principal con los encabezados comunes y el galio local.
Conservar `compile.log`, listado y `build.json` para diagnóstico.
Los logs serie siguen en `logs/` en la raíz. Cerrar el monitor antes de cargar.

Los ajustes de equipo opcionales en `config/local.json` raíz se aplican primero;
`<app>/config/local.json` tiene prioridad. No versionar credenciales.
Para clientes privados, partir de una copia o fork del repo y mantener comunes,
encabezados y herramientas junto a las apps necesarias.
