# Protolink V2 – Header de configuración global (`protolink.h`)

Este archivo define la **configuración base de hardware** para Protolink V2 sobre el PIC18F67K40, incluyendo:

- Mapeo de pines (botones, LEDs, UARTs, RS-485, optoentradas, salidas ULN2003, I2C, etc.).
- Configuración por defecto de puertos (`TRISx`) e I/O.
- Canal de debug serie estándar.
- Infraestructura opcional para **conteo de pulsos por segundo** usando:
  - **INT0 / INT1** (interrupciones externas).
  - **SMT1 / SMT2** (Signal Measurement Timer) como contadores de pulsos.
  - **Timer0** como generador de ventana aproximada de 1 segundo.

Todo esto se activa de manera centralizada, pensado para reutilizarse en todos los firmwares Protolink V2.

---

## 1. MCU, reloj y fuses

El archivo asume:

- MCU: `PIC18F67K40`
- Fuses: `HS` (oscilador de alta velocidad externo)
- Configuración de reloj:

```c
#include <18F67K40.h>
#device ADC = 10
#fuses HS
#use delay(crystal=16mhz, clock=64mhz)
```


## 2. Mapeo de pines Protolink V2
### 2.1. Botón de usuario y LEDs
```c
#define USER   PIN_B2   // Interrupción 2
#define LED1   PIN_F0
#define LED2   PIN_F1

```
USER: botón de usuario.

LED1, LED2: indicadores de estado.

### 2.2. Ethernet (UART1) y reset
```c
#define TX_1   PIN_C6   // UART1 TX
#define RX_1   PIN_C7   // UART1 RX
#define ETH_RST PIN_D7

```
UART1 se usa para el módulo Ethernet.

ETH_RST: salida de reset al módulo Ethernet.

### 2.3. Headers RS-232
```c
#define TX_2   PIN_G1   // UART2 TX
#define RX_2   PIN_G2   // UART2 RX
#define TX_3   PIN_E0   // UART3 TX
#define RX_3   PIN_E1   // UART3 RX
```

UARTs dedicados para puertos RS-232 en headers externos.

### 2.4. Header RS-485
```c
#define TX_4       PIN_C0   // UART4 TX
#define RX_4       PIN_C1   // UART4 RX
#define TX_ENABLE  PIN_H0   // Enable driver RS485
#define RX_ENABLE  PIN_H1   // Enable receiver RS485

```
UART4 para bus RS-485.

TX_ENABLE / RX_ENABLE controlan el transceiver (dirección TX/RX).

### 2.5. USB Serial / IDC-10 (UART5)
```c
#define TX_5   PIN_E2   // UART5 TX
#define RX_5   PIN_E3   // UART5 RX
```

Se usa como puerto de debug principal.

### 2.6. EEPROM y RTC / ADC auxiliar
```c
#define EE_WP    PIN_G7   // Write protect de EEPROM
#define RTC_INT  PIN_B3   // Interrupción RTC
#define ADC_RDY  PIN_F7   // Señal de alerta/ready de ADC (por implementar)
```
### 2.7. Header de expansión (I2C + GPIO)
```c
#define PIN_SDA  PIN_C4   // I2C SDA general
#define PIN_SCL  PIN_C3   // I2C SCL general
#define GPIO_1   PIN_F2
#define GPIO_2   PIN_F3
```

I2C1 está mapeado a PIN_SCL / PIN_SDA.

GPIO_1 y GPIO_2 libres para funciones varias.

### 2.8. Salidas ULN2003 (relevadores / cargas)
```c
#define OUT_0    PIN_D0
#define OUT_1    PIN_D1
#define OUT_2    PIN_D2
#define OUT_3    PIN_D3

```
Salidas de colector abierto a través de ULN2003, típicamente para relevadores u otras cargas discretas.

### 2.9. Entradas optoacopladas
```c
#define IN_0      PIN_B0  // INT0
#define IN_1      PIN_B1  // INT1
#define IN_2      PIN_E5  // CCP1 & (SMT1SIG)
#define IN_3      PIN_E4  // CCP2 & T3CKI
#define IN_2_CCP  PIN_E5
#define IN_3_CCP  PIN_E4
#define IN_2_CKI  PIN_A4
#define IN_3_CKI  PIN_B5
```


IN_0, IN_1: entradas opto con soporte de interrupciones externas INT0 / INT1.

IN_2, IN_3: entradas opto asociadas a CCP y fuentes de clock de timers, ideales para medición de pulsos/frecuencia (cuentalitros, etc.).

Aliases extra (IN_2_CCP, IN_3_CCP, IN_2_CKI, IN_3_CKI) permiten reutilización flexible según el periférico.

## 3. PPS (Peripheral Pin Select)

El archivo ya configura los periféricos clave:
```
// UARTs
#pin_select U1TX = TX_1
#pin_select U1RX = RX_1
#pin_select U2TX = TX_2
#pin_select U2RX = RX_2
#pin_select U3TX = TX_3
#pin_select U3RX = RX_3
#pin_select U4TX = TX_4
#pin_select U4RX = RX_4
#pin_select U5TX = TX_5
#pin_select U5RX = RX_5

// I2C1
#pin_select SCL1IN = PIN_SCL
#pin_select SDA1IN = PIN_SDA

// SMT
#pin_select SMT1SIG = PIN_E5   // SMT1 vinculado a IN_2
#pin_select SMT2SIG = PIN_G7   // SMT2 (compartido con EE_WP si se usa)
```

Nota: SMT1 está conectado a IN_2 (E5), lo que permite usar esa entrada opto como canal de conteo de pulsos por hardware vía SMT1.

## 4. Canal de debug serie
```c
#use rs232(baud=115200, parity=N, UART5, bits=8, stream=DEBUG, errors)

#define protolink_debug_msg(msg)          fprintf(DEBUG, msg)
#define protolink_debug_data(msg, data)   fprintf(DEBUG, msg, data)

```
Debug por UART5 @ 115200 bps.

Macros para logging rápido:
```c
protolink_debug_msg("texto\r\n");

protolink_debug_data("Valor: %lu\r\n", valor);
```

## 5. Inicialización de I/O (protolink_io_init())
```c
void protolink_io_init(void)
{
   // Puertos a "modo seguro" al inicio
   set_tris_a(0xFF);
   set_tris_b(0xFF);

   // C: UART1, UART4, I2C
   set_tris_c(0b10111110);

   // D: salidas ULN2003 + ETH_RST
   set_tris_d(0b01110000);

   // E: UART3, UART5, entradas opto
   set_tris_e(0b11111010);

   // F: LEDs + GPIO + ADC_RDY
   set_tris_f(0b11110000);

   // G: UART2 + EEPROM WP
   set_tris_g(0b01111101);

   // H: control RS485
   set_tris_h(0b11111100);

   output_float(PIN_SDA);
   output_float(PIN_SCL);
}

```
Define la direccionalidad de todos los puertos de Protolink V2.

Deja I2C en estado “float” para permitir uso correcto de open-drain.

## 6. Bloque de funciones por defecto (PROTOLINK_DEFAULT)

Si se define el macro:
```c
#define PROTOLINK_DEFAULT
#include "protolink.h"
```

se habilita un conjunto de funciones y variables globales orientadas a:

Generar una base de tiempo aproximada de 1 segundo usando Timer0.

Contar pulsos por segundo de:

INT0 / INT1 (entradas opto IN_0 e IN_1).

SMT1 / SMT2 como contadores de pulsos hardware.

Disponer de un helper tipo protolink_one_second() para lógica periódica.

### 6.1. Timer0 como generador de “~1 segundo”

Variables:
```c
int16 NInts = 488;     // Número de interrupciones de TMR0 para ~1 s
int16 C_Ints = 0;      // Contador de interrupciones
int1  Flag   = 0;      // Flag que cambia cada ~1 s
int1  lastFlag = 0;    // Estado anterior del flag
```

Con la configuración utilizada:

Timer0 interno, 8 bits, prescaler 1:128.

Cada overflow de Timer0 ≈ 2.048 ms.

488 * 2.048 ms ≈ 999.4 ms (~1 s).

ISR de Timer0
```c
#INT_TIMER0
void TIMER0_isr(void)
{
    C_Ints++;
    if (C_Ints >= NInts)
    {
        C_Ints = 0;
        Flag = !Flag;   // Toggle cada ~1 s

        // Cálculo de pps INT0/INT1
        ...

        // Cálculo de pps SMT1/SMT2
        ...
    }
    clear_interrupt(INT_TIMER0);
}

```
Cada vez que se cumple el conteo de NInts, se:

Togglea Flag.

Actualizan los pulsos por segundo de INT0/INT1 y SMT1/SMT2.

Inicialización de Timer0
```c
void protolink_timer0_init(void)
{
    setup_timer_0(T0_INTERNAL | T0_DIV_128 | T0_8_BIT);

    C_Ints = 0;
    Flag   = 0;

    clear_interrupt(INT_TIMER0);
    enable_interrupts(INT_TIMER0);
    enable_interrupts(GLOBAL);
    set_timer0(0);
}
```
### 6.2. Conteo por INT0 / INT1

Variables:
```
volatile unsigned int32 flow1_counter = 0; // Conteo total INT0
volatile unsigned int32 flow2_counter = 0; // Conteo total INT1

volatile unsigned int32 flow1_pps = 0;     // Pulsos/seg (INT0)
volatile unsigned int32 flow2_pps = 0;     // Pulsos/seg (INT1)

```
ISRs:
```c
#INT_EXT
void isr_int0(void) { flow1_counter++; }

#INT_EXT1
void isr_int1(void) { flow2_counter++; }

```
En la ISR de Timer0, se calcula:
```c
flow1_pps = c1 - last1;
flow2_pps = c2 - last2;

```
donde c1/c2 son las cuentas actuales y last1/last2 las anteriores.

Inicialización de interrupciones externas
```c
void protolink_interrupts_init(void)
{
    ext_int_edge(0, L_TO_H);  // INT0 flanco de subida
    ext_int_edge(1, L_TO_H);  // INT1 flanco de subida

    clear_interrupt(INT_EXT);
    clear_interrupt(INT_EXT1);

    enable_interrupts(INT_EXT);
    enable_interrupts(INT_EXT1);
}
```
### 6.3. Conteo por SMT1 / SMT2

Variables:
```c
volatile unsigned int32 flow1_smt_pps = 0; // Pulsos/seg medidos por SMT1
volatile unsigned int32 flow2_smt_pps = 0; // Pulsos/seg medidos por SMT2
```

Inicialización de SMT:
```c

void protolink_smt_init(void)
{
   setup_smt1(
       SMT_ENABLED
       | SMT_TMR_ROLLSOVER
       | SMT_SIG_ACTIVE_HIGH
       | SMT_TMR_INCREMENTS_ON_RE
       | SMT_DIV_BY_1
       | SMT_REPEAT_DATA_ACQ_MODE
       | SMT_MODE_COUNTER
       | SMT_CLK_FOSC_DIV_4
       | SMT_START_NOW
   );
   smt1_reset_timer();

   setup_smt2(
       SMT_ENABLED
       | SMT_TMR_ROLLSOVER
       | SMT_SIG_ACTIVE_HIGH
       | SMT_TMR_INCREMENTS_ON_RE
       | SMT_DIV_BY_1
       | SMT_REPEAT_DATA_ACQ_MODE
       | SMT_MODE_COUNTER
       | SMT_CLK_FOSC_DIV_4
       | SMT_START_NOW
   );
   smt2_reset_timer();
}
```

En la ISR de Timer0, se leen los contadores:
```c
unsigned int32 smt1_now = smt1_read(SMT_TMR_REG);
unsigned int32 smt2_now = smt2_read(SMT_TMR_REG);

flow1_smt_pps = smt1_now - smt1_last;
flow2_smt_pps = smt2_now - smt2_last;

smt1_last = smt1_now;
smt2_last = smt2_now;
```

6.4. Helper de “evento cada segundo”
```c
int1 protolink_one_second(void)
{
    if (Flag != lastFlag)
    {
        lastFlag = Flag;
        return true;
    }
    return false;
}

```
Uso típico:
```c
if (protolink_one_second())
{
    // Aquí se ejecuta una vez por segundo aprox.
    // Ej: imprimir pps, calcular caudal, etc.
}
```
## 7. Reserva de memoria para bootloader
```c
#define LOADER_END   0x5FF  // ajuste de memoria para protolink v2 bootloader

```
Define el final del área ocupada por el bootloader Protolink V2.

El firmware de aplicación debe respetar esta región (configuración de linker / #build).

## 8. Flujo típico de uso en un proyecto

Ejemplo mínimo de uso con defaults:
```c
#define PROTOLINK_DEFAULT
#include "protolink.h"

void main(void)
{
    protolink_io_init();
    protolink_timer0_init();
    protolink_interrupts_init();
    protolink_smt_init();

    while(TRUE)
    {
        if (protolink_one_second())
        {
            protolink_debug_data("INT0: %lu pps, INT1: %lu pps\r\n", flow1_pps, flow2_pps);
            protolink_debug_data("SMT1: %lu pps, SMT2: %lu pps\r\n", flow1_smt_pps, flow2_smt_pps);
        }

        // Aquí va la lógica de tu aplicación (Modbus, MQTT, etc.)
    }
}
```

## 9. Consideraciones y reservas de periféricos

Al activar PROTOLINK_DEFAULT, este header toma control de:

Timer0 (base de tiempo ~1 s).

INT0 / INT1 (entradas de pulsos en IN_0 / IN_1).

SMT1 / SMT2 (conteo de pulsos).

UART5 como canal de debug (DEBUG).

Si tu aplicación requiere alguno de estos periféricos para otro propósito, deberás:

Ajustar la configuración en este header, o

Deshabilitar PROTOLINK_DEFAULT y gestionar tu propia inicialización.
