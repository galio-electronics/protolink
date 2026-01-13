# Mapa Modbus – Protolink I/O Slave

## 1. General

- **Protocolo:** Modbus RTU
- **Rol:** Esclavo
- **Velocidad recomendada:** 115200 bps (configurable en firmware)
- **Paridad:** NONE (configurable si se requiere)
- **Stop bits:** 1

La numeración "humana" con prefijos 0x/1x/3x/4x corresponde a:
- Coils (0x)           → área `coils[]` (índice 0 = 00001)
- Discrete Inputs (1x) → área `di[]`    (índice 0 = 10001)
- Input Registers (3x) → área `inreg[]` (índice 0 = 30001)
- Holding Registers (4x) → `holdreg[]` (índice 0 = 40001)

## 2. Discrete Inputs (1x) – Lectura de entradas digitales

| Dirección (1x) | Índice interno | Descripción              |
|----------------|----------------|--------------------------|
| 10001          | di[0]          | DI1 estado               |
| 10002          | di[1]          | DI2 estado               |
| 10003          | di[2]          | DI3 estado               |
| 10004          | di[3]          | DI4 estado               |
| 10005–10032    | di[4..31]      | Reservado / expansión    |

## 3. Coils (0x) – Control de salidas a relevador

| Dirección (0x) | Índice interno | Descripción               |
|----------------|----------------|---------------------------|
| 00001          | coils[0]       | DO1 (relevador 1)         |
| 00002          | coils[1]       | DO2                       |
| 00003          | coils[2]       | DO3                       |
| 00004          | coils[3]       | DO4                       |
| 00005–00032    | coils[4..31]   | Reservado / expansión     |

## 4. Input Registers (3x) – Medidas analógicas y diagnóstico

### AI1

| Dirección (3x) | Índice interno   | Descripción                       |
|----------------|------------------|-----------------------------------|
| 30001          | inreg[0]         | AI1 valor bruto ADC               |
| 30002          | inreg[1]         | AI1 corriente mA \* 100           |
| 30003          | inreg[2]         | AI1 valor escalado (ej. 0–1000)   |

### AI2

| Dirección (3x) | Índice interno   | Descripción                       |
|----------------|------------------|-----------------------------------|
| 30011          | inreg[10]        | AI2 valor bruto ADC               |
| 30012          | inreg[11]        | AI2 corriente mA \* 100           |
| 30013          | inreg[12]        | AI2 valor escalado                |

### Diagnóstico / Telemetría

| Dirección (3x) | Índice interno   | Descripción                                  |
|----------------|------------------|----------------------------------------------|
| 30021          | inreg[20]        | Temperatura interna \[°C \* 10]              |
| 30022          | inreg[21]        | Voltaje de bus \[mV]                         |
| 30023          | inreg[22]        | Corriente de bus \[mA]                       |
| 30024–30128    | inreg[23..127]   | Reservado / diagnóstico futuro               |

## 5. Holding Registers (4x) – Configuración del equipo

### Configuración general

| Dirección (4x) | Índice interno     | Descripción                                   |
|----------------|--------------------|-----------------------------------------------|
| 40001          | holdreg[0]         | Modo de operación (0=Normal,1=Calibración)    |
| 40002          | holdreg[1]         | Tiempo de muestreo \[ms]                      |
| 40003          | holdreg[2]         | Filtro AI1 habilitado (0/1)                   |
| 40004          | holdreg[3]         | Filtro AI2 habilitado (0/1)                   |
| 40005          | holdreg[4]         | Timeout de comunicación \[s]                  |

### Calibración AI1

| Dirección (4x) | Índice interno     | Descripción                                   |
|----------------|--------------------|-----------------------------------------------|
| 40011          | holdreg[10]        | Offset AI1 (signed)                           |
| 40012          | holdreg[11]        | Ganancia AI1 (\*1000)                         |

### Calibración AI2

| Dirección (4x) | Índice interno     | Descripción                                   |
|----------------|--------------------|-----------------------------------------------|
| 40021          | holdreg[20]        | Offset AI2 (signed)                           |
| 40022          | holdreg[21]        | Ganancia AI2 (\*1000)                         |

### Configuración salidas DO

| Dirección (4x) | Índice interno     | Descripción                                  |
|----------------|--------------------|----------------------------------------------|
| 40031          | holdreg[30]        | DO1 modo (0=Manual,1=Auto)                   |
| 40032          | holdreg[31]        | DO2 modo                                     |
| 40033          | holdreg[32]        | DO3 modo                                     |
| 40034          | holdreg[33]        | DO4 modo                                     |

## 6. Funciones Modbus soportadas

- 0x01 – Read Coils
- 0x02 – Read Discrete Inputs
- 0x03 – Read Holding Registers
- 0x04 – Read Input Registers
- 0x05 – Write Single Coil
- 0x0F – Write Multiple Coils
- 0x06 – Write Single Register
- 0x10 – Write Multiple Registers
