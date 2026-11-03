# Plantilla Station para cliente

Abrir `station-client.code-workspace` en esta carpeta. Editar `client_rules.c`
para agregar reglas y `app_config.h` para nombre/versión. La fuente compilada es
este `main.c`; las tareas Cliente fijan el perfil `station-client`.

Ctrl+Shift+B compila. Terminal > Ejecutar tarea > Cliente: Compilar, subir y
monitorear carga por auto-boot y muestra los mensajes. Ctrl+C libera el puerto.

Las librerías propias y CCS están disponibles mediante rutas de inclusión,
pero los periféricos opcionales no se activan automáticamente. Consultar
`../../../docs/development.md` y `../../../docs/dependencies.md` antes de habilitarlos.
