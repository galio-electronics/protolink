# Demo base Station

Ejemplo con consola UART1, lectura de entradas y LED periódico usando el scheduler
común. No requiere dependencias externas.

1. Abrir `station-demo.code-workspace` y ejecutar Compilar.
2. Seleccionar COM y ejecutar Compilar, subir y monitorear para probar en placa.
3. Copiar esta carpeta a `firmware/apps/mi-app`, sin `build` ni `galio`.
4. Cambiar `APP_NAME` y `FW_VERSION` en `app_config.h`.
5. Renombrar el workspace y sus etiquetas; conservar las rutas relativas.
6. Cambiar `active_profile` y la clave de `profiles` en `config/project.json`.
7. Agregar repositorios en `config/dependencies.json` siguiendo la
   [guía de dependencias](../../../docs/dependency-setup.md).

Inicializar sensores en `app_init` y atenderlos en `app_task_10ms`,
`app_task_100ms` o `app_task_1000ms` sin bloqueos prolongados.
Configurar los drivers antes de sus includes y respetar UART, timers y buses.
La carpeta `galio/` se crea al preparar las dependencias.

Para clientes privados, copiar o bifurcar el repositorio completo: esta app usa
los encabezados de placa, `firmware/common` y `tools` mediante rutas relativas.
No necesita registrarse en ningún archivo central.
