# Station Blink

Demo con `main.c` propio que repite la secuencia **LED1 → LED2 → LED3 → LED4**.
Cada LED se enciende durante 250 ms, seguido de 250 ms con todos apagados.
Solo se enciende un LED a la vez; después de LED4 vuelve a LED1.

Abrir `station-blink.code-workspace` y ejecutar **Compilar**, o desde la raíz:

```powershell
./tools/run.ps1 build --profile station-blink
```

Cambiar `BLINK_INTERVAL_MS` en `main.c` para ajustar el tiempo de encendido y
apagado. Usa el temporizador de la placa sin esperas bloqueantes y respeta
`STATION_LED_ACTIVE_MASK`. No requiere dependencias externas ni consola serie.
