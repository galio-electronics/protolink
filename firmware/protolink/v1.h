/* 
 * File:   protolink.h
 * Author: Arturo Gasca
 * Galio Electronics
 * Protolink V1 Pinout
 *
 * Created on 12 de noviembre de 2025, 08:25 PM
 */

#ifndef PROTOLINK_H
#define	PROTOLINK_H

#include <18F67K40.h>          // Ajusta tu MCU
#device ADC = 10
#fuses HS
//#use delay(crystal=20mhz)
#use delay(crystal=16mhz, clock=64mhz)

/*USER BUTTON AND LEDS*/
#define USER        PIN_B2  //Interruption 2
#define LED1        PIN_F0
#define LED2        PIN_F1


/*HEADER RS232*/
#define TX_2        PIN_G1  //Puerto 1 UART2
#define RX_2        PIN_G2
#define TX_3        PIN_E0  //Puerto 2 UART3
#define RX_3        PIN_E1 

/*HEADER RS485*/
#define TX_4        PIN_C0  //UART4 
#define RX_4        PIN_C1
#define TX_ENABLE   PIN_H0  //Internal Auxiliar
#define RX_ENABLE   PIN_H1  //Internal Auxiliar




/*EEPROM Write Protect*/
#define EE_WP       PIN_G7
/*RTC AUX*/
#define RTC_INT     PIN_B3  //Interruption 3


/*ACCESORY EXPANSION HEADER*/
#define PIN_SDA     PIN_C4  //I2C General Board
#define PIN_SCL     PIN_C3  //I2C General Board
#define GPIO_1      PIN_F2
#define GPIO_2      PIN_F3


/*INPUTS OPTOS*/
#define IN_0        PIN_B0  //Interruption 0
#define IN_1        PIN_B1  //Interruption 1

/*GPIOS*/
#define ANALOG_0    PIN_A0
#define ANALOG_1    PIN_A1
#define PWM1        PIN_E5
#define PWM2        PIN_E4

//UART1
//#pin_select U1TX=TX_1
//#pin_select U1RX=RX_1

//UART2
#pin_select U2TX=TX_2
#pin_select U2RX=RX_2

//UART3
#pin_select U3TX=TX_3
#pin_select U3RX=RX_3

//UART4
#pin_select U4TX=TX_4
#pin_select U4RX=RX_4

//UART5
//#pin_select U5TX=TX_5
//#pin_select U5RX=RX_5

//I2C1
#pin_select SCL1IN=PIN_SCL
#pin_select SDA1IN=PIN_SDA


//DEBUG SERIAL
#use rs232(baud=115200,parity=N,UART3,bits=8,stream=DEBUG,errors)

#define protolink_debug_msg(msg) fprintf(DEBUG, msg)
#define protolink_debug_data(msg,data) fprintf(DEBUG,msg,data)

void protolink_io_init(void)
{
   // Primero todo como entrada (safe mode)
   set_tris_a(0xFF);   // si no usas A, d?jalo as?
   set_tris_b(0xFF);   // B todo entrada (bot?n / optos / RTC_INT)
   
   // C: UART1, UART4, I2C
   set_tris_c(0b10111110);   // C7 RX1 in, C6 TX1 out, C1 RX4 in, C0 TX4 out, SCL/SDA in

   // D: salidas ULN2003 + ETH_RST
   set_tris_d(0b01110000);   // D3..D0 out, D7 out, D6..D4 in

   // E: UART3, UART5, entradas opto
   set_tris_e(0b11111010);   // E0/E2 out, resto in

   // F: LEDs + GPIO + ADC_RDY
   set_tris_f(0b11110000);   // F0/F1/F2/F3 out, resto in

   // G: UART2 + EEPROM WP
   set_tris_g(0b01111101);   // G1 TX2 out, G2 RX2 in, G7 EE_WP out

   // H: control RS485
   set_tris_h(0b11111100);   // H0/H1 out, resto in
   
   output_float(PIN_SDA);
   output_float(PIN_SCL);
}


#define LOADER_END   0x5FF  //ajuste de memoria para protolink v2 bootloader
#endif	/* PROTOLINK_H */


