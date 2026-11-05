> Registro histórico: el firmware y workspace maestros fueron retirados. Las rutas mencionadas reflejan las pruebas de entonces.

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

## Optimización de carga y LCD — 2 de noviembre de 2026

Se corrigió el ejemplo añadido por el usuario: inicialización LCD en `app_init`
del cliente (después de GPIO), dirección de escritura 0x4E derivada de 0x27,
stream I2C explícito a 100 kHz y texto repartido en dos filas de 16 columnas.
La API RAM del driver respeta la opción CCS PASS_STRINGS=IN_RAM.
El usuario confirmó que el mensaje se ve correctamente. El driver externo
lcd_i2c permanece intacto y sus huellas coinciden con el manifiesto.

Compilación del ejemplo cliente: 4570 bytes de aplicación, 290 registros Intel HEX
originales, 12 bytes de fuses excluidos; 0 errores y 0 advertencias.
Se midió el mismo HEX antes y después de optimizar, en COM26 a 115200:

| Métrica | Antes | Después |
| --- | --- | --- |
| Registros enviados | 288 | 181 |
| Bytes serie del HEX normalizado | 12612 | 11328 |
| Transferencia con confirmaciones | 2.788 s | 2.064 s |
| Abrir puerto + auto-boot + transferencia | 5.215 s | 3.518 s |

Reducción medida de entrada+carga: 32.5%. Estos tiempos excluyen compilación,
el segundo de reinicio interno posterior a EOF y el tiempo de monitor.

Cambios del cargador: registros de hasta 26 bytes (64 caracteres incluido CR),
sin cruzar filas flash de 128 bytes; eliminación de flush por registro porque
el ACK ya confirma recepción; lectura de XOFF/ACK/XON en bloque con soporte de
fragmentación; espera de 1050 ms contada desde la recepción del banner, descontando
el tiempo ya transcurrido. No se modificó baudrate, protocolo ni bootloader.

Pasaron 12 pruebas, incluida reconstrucción del payload y comprobación de tamaños,
alineación y límites de fila. El maestro también recompiló sin errores: 3328 bytes,
213 líneas HEX originales, 133 registros enviados con el nuevo empaquetado.
La placa quedó con el ejemplo cliente LCD; el perfil predeterminado del workspace
principal continúa siendo station-master. El workspace Cliente fija station-client.
