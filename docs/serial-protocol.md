# Diagnóstico serie Station 0.1.1

UART1 a 115200 8N1, RC6 TX y RC7 RX, compartida en el tiempo con el bootloader.
No usar RTS/CTS ni DSR/DTR automáticos. DTR/RTS se reservan para auto-boot.
La consola no es Modbus y no interpreta registros HEX.

Mensajes de ejemplo:

```text
INFO app=station-master fw=0.1.1 build=2-Nov-26 21:54:28 reset=0x0330
STAT t=1000 di_raw=0x00 tx_drop=0 rx_drop=0
EVENT client_inputs=0x01
```

`t` es el contador unsigned de milisegundos, que vuelve a cero tras reset o al
completar su rango. `di_raw` son niveles de MCU, bit 0=DI1, bit 7=DI8; no se asume
polaridad lógica. `reset` es el valor crudo de `restart_cause()` CCS.
`tx_drop` y `rx_drop` cuentan caracteres descartados por llenado de las colas,
no todos los posibles errores eléctricos de UART. Los contadores son de 16 bits.

`info` devuelve identificación. `status`, `inputs` y `errors` devuelven el estado
actual; `help` u otro comando muestra la lista. Terminación CR o LF; máximo 23
caracteres, comandos en minúsculas. Las líneas más largas se rechazan. No hay
comandos de escritura ni reset remoto en esta etapa.

TX: cola de 128 bytes (127 utilizables), consumida por INT_TBE. RX: 64 bytes (63
utilizables), alimentada por INT_RDA. No se hace printf desde ISR ni se espera a
UART desde main. `printf(diagnostics_putc,...)` formatea y encola; si la cola se
llena descarta caracteres, por lo que una línea puede quedar incompleta. Limitar
frecuencia/longitud de logs y observar contadores. El parser procesa un carácter
por vuelta y las respuestas se producen en main.

El monitor añade hora local del PC, guarda en `logs/` y permite captura acotada.
Usar `Ctrl+C` para liberar COM antes de iniciar otra tarea. Durante carga, el mismo
handle recibe ACK/XON y después pasa inmediatamente a captura de aplicación.
No se purga RX tras EOF ni se cierra/reabre el puerto para cambiar a monitor.

El monitor habilita envío de comandos solo después de observar INFO/STAT de aplicación, para no enviar texto de consola al parser HEX del bootloader.
