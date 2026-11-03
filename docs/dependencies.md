# Dependencias y propiedad de recursos

Las carpetas externas son dependencias fuente compartidas. No se movieron ni
modificaron. `config/dependencies.lock.json` registra SHA256 de sus `.c/.h`
(excluyendo ejemplos), porque no se depende de que cada carpeta tenga metadata Git.
`Station: Verificar dependencias` detecta ausencias o diferencias. El build anota
esas diferencias pero no bloquea trabajo deliberado sobre una librería.
Para adoptar cambios, revisar, probar y actualizar las huellas intencionalmente.

| Dependencia | Inclusión actual | Observaciones de integración |
| --- | --- | --- |
| CCS 5.081 | `<18F67K40.h>`, `<bootloader.h>`, etc. | Instalación externa licenciada; no editarla/copiarla sin necesidad |
| simple_master | `<simple_master/simple_master.c>` | `.c` incluye `.h`; default UART1, cambiar explícitamente; hay esperas bloqueantes y `smodbus_rx_flush` refiere INT_RDA4 directamente |
| rui3_at | `<rui3_at/rui3_at.h>` | `.h` incluye `.c`; no incluir ambos; default UART1, buffers grandes y llamadas con espera; revisar selección de UART y tamaño de índices |
| lcd_i2c | `<lcd_i2c/lcd_i2c.c>` | Configurar dirección y macros del bus; operaciones de pantalla contienen retardos |
| oled_i2c | `<oled_i2c/oled_ssd1306.h>` | `.h` incluye `.c`; requiere stream `OLED_I2C`, geometría/dirección del módulo y recursos de fuentes |
| expio_23ioxx | Fuentes existentes en `firmware/examples/protolink-expio/` | Depende de simple_master y hooks del mapa Modbus; ejemplo histórico, no integrado en Station |

Las copias de `v1.h/v2.h` en ejemplos externos son históricas; no usarlas para
Station. Incluir siempre el `station.h` del repositorio. Las rutas antiguas del
ejemplo EXPIO no son la configuración de los nuevos perfiles.

## Recursos reservados

| Recurso | Propietario actual |
| --- | --- |
| UART1 RC6/RC7 | Bootloader durante carga; consola de aplicación durante ejecución |
| INT_RDA / INT_TBE | Diagnóstico UART1, RX y TX respectivamente |
| Timer2 / INT_TIMER2 | Tick de sistema 1 ms |
| UART4 RC0/RC1 | Reserva para RS485 futuro; no activada |
| RC3/RC4 | Reserva I2C; no inicializado en firmware maestro |
| UART2/3/5 y SPI | Disponibles según cableado confirmado; sin activación automática |

Los flags de `library_config.h` son puntos de integración, no una promesa de que
las cuatro librerías pueden ejecutarse juntas sin adaptar recursos. Debe validarse
cada incorporación y el presupuesto RAM del PIC. Se preservaron los fuentes
externos y no se declara validación funcional de LoRa, Modbus, LCD u OLED.

Para nuevas librerías compartidas, usar carpeta/repositorio hermano con `.h`, `.c`,
README y ejemplos. Agregar al workspace, rutas de inclusión y manifiesto. Para una
función exclusiva de cliente usar un módulo dentro de su aplicación.
