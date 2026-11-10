# Contrato de hardware Station

PIC18F67K40, cristal 16 MHz con PLL a 64 MHz. Mapa único en `station.h`.
DO1–DO8 inactivas en alto (confirmación del usuario); LEDs activos en alto.
Los LAT se precargan con pines como entradas antes de habilitar DO/LED mediante TRIS.
ADC deshabilitado en la inicializacion base; cada app habilita los canales que
utiliza (station-rak3172 0.2.7 habilita VIN en RA1/AN1 y AUX en RA0/AN0). Entradas digitales crudas. Controles de módulos
sin activar: su nivel durante reset depende de la polarización de hardware.

BOOT RA4 activo bajo, leído al arrancar. UART1 RC6/RC7 y CP210x COM26.
El circuito usa dos NPN cruzados; ver `autoboot.md`. No tratar DTR/RTS como GPIO
independientes de BOOT/RESET.

Memoria: loader 0x0000..0x05FF, reset de aplicación 0x0600, interrupción 0x0608.
No modificar esta reserva al desarrollar aplicaciones. `_bootloader` solo para
compilar un bootloader, nunca en un perfil de aplicación. No cargar el HEX de
bootloader con el script de aplicación: se rechaza deliberadamente.

El bootloader compartido Station/Monitor se conserva. No se ha desarrollado
aplicación Monitor. Encabezados V1/V2/Excore conservan sus rutas y contenido.

Lista de pruebas al agregar periféricos:

- Comprobar PPS, TRIS (fast_io), polaridad, alimentación y propietario de UART/ISR.
- Revisar tiempos bloqueantes y RAM de buffers, y no usar UART1 para otro driver.
- Compilar, revisar `.lst`, cargar y confirmar INFO/STAT y versión esperada.
- Medir salidas durante reset/bootloader/aplicación; el código no sustituye pulls.
- Probar reinicio y funcionamiento sin módulos opcionales instalados.
