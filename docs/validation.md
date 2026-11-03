# Validación — 2 de noviembre de 2026

Se ejecutaron las tareas mediante `tools/run.ps1`, el mismo lanzador de VS Code.
No se automatizó la interfaz gráfica del editor; sí los comandos que usan sus tareas.

| Perfil | CCS 5.081 | ROM | RAM máxima | Prueba en placa |
| --- | --- | --- | --- | --- |
| station-master | 0 errores, 0 advertencias | 3328 bytes | 343 bytes | Carga, INFO, STAT y comandos info/status |
| station-client | 0 errores, 0 advertencias | 3462 bytes | 344 bytes | Carga, INFO, EVENT y comando status |
| station-blink | 0 errores, 0 advertencias | 912 bytes | 57 bytes | Parpadeo confirmado previamente por usuario |

Se dejó **station-master** seleccionado y cargado al terminar. COM26 quedó cerrado
por el monitor para que VS Code pueda abrirlo. UART1 a 115200 8N1.

Capturas locales (ignoradas por Git):

- `logs/20261102-220030-260050-station-client.log`
- `logs/20261102-220145-526697-station-master.log`

Los registros de prueba muestran tx_drop=0 y rx_drop=0. El bootloader aceptó
211 registros para el maestro y 219 para la plantilla, con ACK final; se recibió
la identificación de aplicación después del reset sin cerrar/reabrir COM.
No hay lectura de verificación de flash en el protocolo existente.

11 pruebas de Python pasaron: validador HEX, checksum, reserva de bootloader,
EOF, solapamientos, cambio de banco, respuestas de carga, secuencia cruzada,
selección de main.c, matcher de errores y continuidad del puerto hasta monitor.
Las huellas de las cuatro dependencias externas coinciden con el manifiesto.

El bootloader se recompiló para comprobar compatibilidad con station.h actualizado:
0 errores, 1 advertencia heredada (205 en loader común). No se volvió a grabar por
ICSP ni se cambió su reserva de memoria.

No se han validado funcionalmente Modbus, LoRa, LCD ni OLED en esta etapa. Sus
fuentes están disponibles en el workspace y sus puntos de integración documentados.
La consola aporta observabilidad para incorporar cada periférico de manera aislada.
