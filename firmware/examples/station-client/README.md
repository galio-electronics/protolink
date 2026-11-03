# Plantilla Station para cliente

Abrir `station-client.code-workspace` en esta carpeta. Editar `client_rules.c`
para agregar reglas y `app_config.h` para nombre/versión. La fuente compilada es
este `main.c`; las tareas Cliente fijan el perfil `station-client`.

Ctrl+Shift+B compila. Terminal > Ejecutar tarea > Cliente: Compilar, subir y
monitorear carga por auto-boot y muestra los mensajes. Ctrl+C libera el puerto.

Las librerías propias y CCS están disponibles mediante rutas de inclusión,
pero los periféricos opcionales no se activan automáticamente. Consultar
`../../../docs/development.md` y `../../../docs/dependencies.md` antes de habilitarlos.

## LCD I2C habilitado en esta plantilla

`app_config.h` habilita `APP_USE_LCD`, dirección de 7 bits `0x27`, geometría 16x2.
El driver externo espera la dirección de escritura: `LCD_I2C_ADDR = 0x27 << 1`
(0x4E). `main.c` declara el stream `LCD_BUS` en I2C1, RC3=SCL y RC4=SDA, a 100 kHz.
Se reutiliza la librería externa sin modificarla.

`app_init()` se ejecuta después de configurar la placa: comprueba ACK, inicializa
la pantalla y escribe `Galio` en la primera fila y `Electronics` en la segunda.
La cadena completa ocupa 17 caracteres y no cabe en una sola fila de 16.
Se utiliza la API RAM del driver porque esta aplicación habilita
`PASS_STRINGS=IN_RAM`. La inicialización contiene los retardos requeridos por el
driver, pero solo se ejecuta una vez, no desde las tareas periódicas.

El estado serie muestra `INFO lcd_addr=0x27 ack=1 message=Galio_Electronics`.
Sin ACK se informa `WARN ... check_address_wiring` y se omite la inicialización.
La pantalla fue confirmada visualmente por el usuario el 2 de noviembre de 2026.
El scheduler común no contiene llamadas LCD; el perfil maestro sigue sin depender
de este periférico. Para cargar esta pantalla seleccionar `station-client` o usar
las tareas Cliente de este workspace.
