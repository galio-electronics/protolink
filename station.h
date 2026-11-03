/*
 * File:   station.h
 * Galio Electronics
 * Protolink Station Pinout - firmware 0.1.0
 */
#ifndef PROTOLINK_H
#define PROTOLINK_H

#include <18F67K40.h>
#device ADC = 10
#ifdef STATION_PASS_STRINGS_IN_RAM
#device PASS_STRINGS=IN_RAM
#endif
#fuses HS
#fuses NOPPS1WAY
#use delay(crystal=16mhz,clock=64mhz)

#define PROTOLINK_STATION 1
#ifndef FW_VERSION
#define FW_VERSION "0.1.0"
#endif
#define LOADER_END 0x5FF

/*ENTRADAS Y CONTROL*/
#define AUX_ADC     PIN_A0
#define VIN_ADC     PIN_A1
#define DI_1WIRE    PIN_A2
#define ADC_RDY     PIN_A3
#define MCU_BOOT    PIN_A4
#define BUZZ_OUT    PIN_A5
#define OSC2        PIN_A6
#define OSC1        PIN_A7
#define MCU_CLR     PIN_G5
#define ETH_RST     PIN_G6
#define RTC_MFP     PIN_G7
#define MCU_PGC     PIN_B6
#define MCU_PGD     PIN_B7

/*INPUTS: IN_0 corresponde a DI1; bit 0 = DI1*/
#define IN_0        PIN_E7
#define IN_1        PIN_E6
#define IN_2        PIN_E5
#define IN_3        PIN_E4
#define IN_4        PIN_B0
#define IN_5        PIN_B1
#define IN_6        PIN_B2
#define IN_7        PIN_B3

/*OUTPUTS: OUT_0 corresponde a DO1; bit 0 = DO1*/
#define OUT_0       PIN_B5
#define OUT_1       PIN_B4
#define OUT_2       PIN_H3
#define OUT_3       PIN_H2
#define OUT_4       PIN_D0
#define OUT_5       PIN_G0
#define OUT_6       PIN_G3
#define OUT_7       PIN_G4

/*LEDS Y ENTRADAS ANALOGICAS*/
#define LED1        PIN_F4
#define LED2        PIN_F5
#define LED3        PIN_F6
#define LED4        PIN_F7
#define ADR0        PIN_F0
#define ADR1        PIN_F1
#define ADR2        PIN_F2
#define ADR3        PIN_F3

/*UARTS: solo diagnostico se habilita en esta etapa*/
#define TX_1        PIN_C6
#define RX_1        PIN_C7
#define TX_2        PIN_G1
#define RX_2        PIN_G2
#define TX_3        PIN_E0
#define RX_3        PIN_E1
#define TX_4        PIN_C0
#define RX_4        PIN_C1
#define TX_5        PIN_E2
#define RX_5        PIN_E3
#define TX_ENABLE   PIN_H0
#define RX_ENABLE   PIN_H1
#define RS485_STATUS PIN_C2

/*I2C Y SPI: reservados, sin PPS ni driver activo*/
#define PIN_SCL     PIN_C3
#define PIN_SDA     PIN_C4
#define SPI_MOSI    PIN_D4
#define SPI_MISO    PIN_D5
#define SPI_SCK     PIN_D6
#define SPI_CS      PIN_D7
#define SIMCOM_SLP  PIN_D1
#define SIMCOM_PKEY PIN_D2
#define LORA_RST    PIN_D3

/*Polaridades confirmadas por el usuario: alto desactiva DO y enciende LED.
 * Mascara por canal: bit 0 = DO1 / LED1. DI se publica sin invertir.
 */
#ifndef STATION_DO_INACTIVE_MASK
#define STATION_DO_INACTIVE_MASK 0xFF
#endif
#ifndef STATION_LED_ACTIVE_MASK
#define STATION_LED_ACTIVE_MASK 0x0F
#endif
#ifndef STATION_DEBUG_ENABLED
#define STATION_DEBUG_ENABLED 0
#endif
#ifndef STATION_DEBUG_UART
#define STATION_DEBUG_UART 1
#endif
#ifndef STATION_DEBUG_BAUD
#define STATION_DEBUG_BAUD 115200
#endif

#if (STATION_DEBUG_ENABLED == 1)
#if (STATION_DEBUG_UART == 1)
#pin_select U1TX=TX_1
#pin_select U1RX=RX_1
#use rs232(baud=STATION_DEBUG_BAUD,parity=N,UART1,bits=8,stream=DEBUG,errors)
#elif (STATION_DEBUG_UART == 5)
#pin_select U5TX=TX_5
#pin_select U5RX=RX_5
#use rs232(baud=STATION_DEBUG_BAUD,parity=N,UART5,bits=8,stream=DEBUG,errors)
#else
#error STATION_DEBUG_UART debe ser 1 o 5
#endif
#define protolink_debug_msg(msg) fprintf(DEBUG,msg)
#define protolink_debug_data(msg,data) fprintf(DEBUG,msg,data)
#endif

/*TRIS explicito: precargar LAT sin habilitar las salidas prematuramente.*/
#use fast_io(A)
#use fast_io(B)
#use fast_io(C)
#use fast_io(D)
#use fast_io(E)
#use fast_io(F)
#use fast_io(G)
#use fast_io(H)

/*Driver CCS comun: reserva 0x0000..0x05FF y reubica los vectores.*/
#include <bootloader.h>

void protolink_io_init(void)
{
   set_tris_a(0xFF);
   set_tris_b(0xFF);
   set_tris_c(0xFF);
   set_tris_d(0xFF);
   set_tris_e(0xFF);
   set_tris_f(0xFF);
   set_tris_g(0xFF);
   set_tris_h(0xFF);
   setup_adc(ADC_OFF);
   setup_adc_ports(NO_ANALOGS,NO_ANALOGS_P2);

   output_bit(OUT_0,((STATION_DO_INACTIVE_MASK & (1 << 0)) != 0));
   output_bit(OUT_1,((STATION_DO_INACTIVE_MASK & (1 << 1)) != 0));
   output_bit(OUT_2,((STATION_DO_INACTIVE_MASK & (1 << 2)) != 0));
   output_bit(OUT_3,((STATION_DO_INACTIVE_MASK & (1 << 3)) != 0));
   output_bit(OUT_4,((STATION_DO_INACTIVE_MASK & (1 << 4)) != 0));
   output_bit(OUT_5,((STATION_DO_INACTIVE_MASK & (1 << 5)) != 0));
   output_bit(OUT_6,((STATION_DO_INACTIVE_MASK & (1 << 6)) != 0));
   output_bit(OUT_7,((STATION_DO_INACTIVE_MASK & (1 << 7)) != 0));
   output_bit(LED1,!((STATION_LED_ACTIVE_MASK & (1 << 0)) != 0));
   output_bit(LED2,!((STATION_LED_ACTIVE_MASK & (1 << 1)) != 0));
   output_bit(LED3,!((STATION_LED_ACTIVE_MASK & (1 << 2)) != 0));
   output_bit(LED4,!((STATION_LED_ACTIVE_MASK & (1 << 3)) != 0));

   set_tris_b(0b11001111); // DO1/2; DI5..8 e ICSP como entradas
   set_tris_d(0b11111110); // DO5
   set_tris_f(0b00001111); // LED1..4; ADR0..3 como entradas
   set_tris_g(0b11100110); // DO6..8; MCLR y controles sin modificar
   set_tris_h(0b11110011); // DO3/4; enables RS485 sin conducir
#if (STATION_DEBUG_ENABLED == 1)
#if (STATION_DEBUG_UART == 1)
   output_high(TX_1);
   set_tris_c(0b10111111);
#else
   output_high(TX_5);
   set_tris_e(0b11111011);
#endif
   setup_uart(STATION_DEBUG_BAUD,DEBUG);
#endif
}

#ifdef PROTOLINK_DEFAULT
volatile unsigned int32 station_ticks_ms = 0;

#INT_TIMER2
void TIMER2_isr(void)
{
   ++station_ticks_ms;
}

void protolink_timer2_init(void)
{
   disable_interrupts(INT_TIMER2);
   setup_timer_2(T2_DISABLED,249,4);
   station_ticks_ms = 0;
   set_timer2(0);
   // Fosc/4 = 16 MHz; /16 * (249+1) * 4 = 1 ms.
   setup_timer_2(T2_CLK_INTERNAL | T2_DIV_BY_16,249,4);
   clear_interrupt(INT_TIMER2);
   enable_interrupts(INT_TIMER2);
}

// Solo desde main, despues de inicializar Timer2; no llamar desde ISR.
unsigned int32 protolink_millis(void)
{
   unsigned int32 snapshot;
   disable_interrupts(INT_TIMER2);
   snapshot = station_ticks_ms;
   enable_interrupts(INT_TIMER2);
   return snapshot;
}

unsigned int8 protolink_read_inputs(void)
{
   unsigned int8 value = 0;
   if(input(IN_0)) bit_set(value,0);
   if(input(IN_1)) bit_set(value,1);
   if(input(IN_2)) bit_set(value,2);
   if(input(IN_3)) bit_set(value,3);
   if(input(IN_4)) bit_set(value,4);
   if(input(IN_5)) bit_set(value,5);
   if(input(IN_6)) bit_set(value,6);
   if(input(IN_7)) bit_set(value,7);
   return value;
}
#endif
#endif /* PROTOLINK_H */
