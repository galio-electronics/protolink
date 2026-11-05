# Dependencias y propiedad de recursos

Las dependencias Git se descargan automaticamente dentro de `galio/` antes de
compilar. El manifiesto `config/dependencies.json` contiene URLs, commits,
huellas SHA256 y la lista por aplicacion. Los repositorios originales hermanos
no se modifican ni se requieren para los builds nuevos.

Ver [preparacion en otra computadora y actualizaciones](dependency-setup.md).
`dependencies` inspecciona; `deps-sync` descarga lo ausente. Si una copia tiene
cambios locales o no coincide con el manifiesto, el build se detiene sin tocarla.

Excepcion incorporada con `station-qdy30a`: `simple_master` ahora permite 8N1,
espera fin de TX antes de liberar DE y limita la recepcion completa por timeout.
Las huellas actuales incluyen esos cambios; el parche reproducible esta en
`patches/simple-master-station-rs485.patch`. Ver la guia de la demo para alcance
y estado de pruebas fisicas.

| Dependencia | Inclusión actual | Observaciones de integración |
| --- | --- | --- |
| analog_drivers | `<analog_drivers/ads1115.h>` | Repositorio descargado en `galio/analog_drivers`; `station-hw-test` reutiliza las definiciones ADS1115 y usa el puerto I2C acotado. No llama al bucle sin timeout de `ads1115.c`. |
| sd-card | `<sd-card/sd_card.c>` y `<sd-card/fat32.c>` | Solo `station-rtc-sd`; SPI2 dedicado, buffer único de 512 bytes, FAT32 acotada para demo |
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

Para nuevas librerías compartidas, usar un repositorio Git declarado en el manifiesto con `.h`, `.c`,
README y ejemplos. Agregar al workspace, rutas de inclusión y manifiesto. Para una
función exclusiva de cliente usar un módulo dentro de su aplicación.

El ejemplo cliente ahora muestra un mensaje LCD 16x2 por I2C1; su configuración e
inicialización permanecen en la aplicación cliente. La librería PCF8574 se utiliza
a 100 kHz, conforme al [datasheet de TI](https://www.ti.com/lit/ds/symlink/pcf8574.pdf).
La dirección 0x27 de 7 bits se convierte a 0x4E para el driver existente.
