/* 
 * File:   protolink.h
 * Author: Arturo Gasca
 * Galio Electronics
 * Protolink V2 Pinout
 *
 * Created on 12 de noviembre de 2025, 08:25 PM
 */

#ifndef PROTOLINK_H
#define	PROTOLINK_H

#include <18F67K40.h>          // Ajusta tu MCU
#device ADC = 10
#fuses HS
#use delay(crystal=16mhz,clock=64mhz)

/*USER BUTTON AND LEDS*/
#define USER        PIN_B2  //Interruption 2
#define LED1        PIN_F0
#define LED2        PIN_F1

/*ETHERNET*/
#define TX_1        PIN_C6  //UART1
#define RX_1        PIN_C7
#define ETH_RST     PIN_D7

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

/*USB SERIAL/HEADER IDC-10*/
#define TX_5        PIN_E2  //UART5
#define RX_5        PIN_E3

/*EEPROM Write Protect*/
#define EE_WP       PIN_G7
/*RTC AUX*/
#define RTC_INT     PIN_B3  //Interruption 3
/*ADC Alert/Ready*/
#define ADC_RDY     PIN_F7 //Por implementar en PCB

/*ACCESORY EXPANSION HEADER*/
#define PIN_SDA     PIN_C4  //I2C General Board
#define PIN_SCL     PIN_C3  //I2C General Board
#define GPIO_1      PIN_F2
#define GPIO_2      PIN_F3


/*OUTPUTS LN2003*/
#define OUT_0       PIN_D0
#define OUT_1       PIN_D1
#define OUT_2       PIN_D2
#define OUT_3       PIN_D3

/*INPUTS OPTOS*/
#define IN_0        PIN_B0  //Interruption 0
#define IN_1        PIN_B1  //Interruption 1
#define IN_2        PIN_E5  //CCP1  & T0CKI
#define IN_3        PIN_E4  //CCP2  & T3CKI
#define IN_2_CCP    PIN_E5
#define IN_3_CCP    PIN_E4
#define IN_2_CKI    PIN_A4
#define IN_3_CKI    PIN_B5

//UART1
#pin_select U1TX=TX_1
#pin_select U1RX=RX_1

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
#pin_select U5TX=TX_5
#pin_select U5RX=RX_5

//I2C1
#pin_select SCL1IN=PIN_SCL
#pin_select SDA1IN=PIN_SDA


//SMT
#pin_select SMT1SIG = PIN_E7   // SMT1
#pin_select SMT2SIG = PIN_G7   // SMT2

//DEBUG SERIAL
#use rs232(baud=115200,parity=N,UART5,bits=8,stream=DEBUG,errors)

#define protolink_debug_msg(msg) fprintf(DEBUG, msg)
#define protolink_debug_data(msg,data) fprintf(DEBUG,msg,data)


void protolink_io_init(void)
{
   // Primero todo como entrada (safe mode)
   set_tris_a(0xFF);   // si no usas A, dejalo asi
   set_tris_b(0xFF);   // B todo entrada (botï¿½n / optos / RTC_INT)
   
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

#ifdef PROTOLINK_DEFAULT



/* Protolink Default functions
 * Active define for use in main
 * Interrupts use for counters INT0, INT1
 * SMT1 and SMT use for counters
 * Timer0 config for 1 second, consume in protolink_one_second()
 */
// Numero de interrupciones de TIMER0 para ~1 segundo
// 2.048 ms * 488 ? 999.424 ms
int16 NInts = 488;

// VARIABLES GLOBALES
int16 C_Ints = 0;   // Contador de interrupciones ocurridas
int1 Flag = 0;     // Flag que cambia cada ~1 segundo

// Ejemplo de uso: hacer algo cada vez que cambie Flag (~1s)
int1 lastFlag = 0;



volatile unsigned int32 flow1_smt_pps = 0;   // Pulsos/s medidos por SMT1
volatile unsigned int32 flow2_smt_pps = 0;   // Pulsos/s medidos por SMT2


// ---- CONTADORES ----
volatile unsigned int32 flow1_counter = 0;      // Conteo total por INT0
volatile unsigned int32 flow2_counter = 0;      // Conteo total por INT1

volatile unsigned int32 flow1_pps = 0;          // Pulsos por segundo (INT0)
volatile unsigned int32 flow2_pps = 0;          // Pulsos por segundo (INT1)

#INT_TIMER0
void TIMER0_isr(void){
    // Incrementamos primero
    C_Ints++;

    if(C_Ints >= NInts)
    {
        C_Ints = 0;
        Flag = !Flag;   // Toggle cada ~1s
        
        static unsigned int32 last1 = 0;
        static unsigned int32 last2 = 0;
        unsigned int32 c1 = flow1_counter;
        unsigned int32 c2 = flow2_counter;
        flow1_pps = c1 - last1;
        flow2_pps = c2 - last2;
        last1 = c1;
        last2 = c2;
        
         // --- SMT1 / SMT2 ---
        static unsigned int32 smt1_last = 0;
        static unsigned int32 smt2_last = 0;

        unsigned int32 smt1_now = smt1_read(SMT_TMR_REG);
        unsigned int32 smt2_now = smt2_read(SMT_TMR_REG);

        flow1_smt_pps = smt1_now - smt1_last;
        flow2_smt_pps = smt2_now - smt2_last;

        smt1_last = smt1_now;
        smt2_last = smt2_now;
    }
    clear_interrupt(INT_TIMER0);
    

}

// ---- ISR INTERRUPCIONES EXTERNAS ----
#INT_EXT            // INT0
void isr_int0(void)
{
   flow1_counter++;
}

#INT_EXT1           // INT1
void isr_int1(void)
{
   flow2_counter++;
}

//Init default timer 0 for 1 second
void protolink_timer0_init(void){
    // Timer0 interno, 8 bits, prescaler 1:128
    // (Las etiquetas pueden ser T0_* o RTCC_* segun version CCS)
    setup_timer_0(T0_INTERNAL | T0_DIV_128 | T0_8_BIT);

    C_Ints = 0;
    Flag = 0;

    clear_interrupt(INT_TIMER0);
    enable_interrupts(INT_TIMER0);
    enable_interrupts(GLOBAL);
    set_timer0(0);
    
}
//Init default Interrupts for protolink
void protolink_interrupts_init(void){
    // INT0 en flanco de subida
    ext_int_edge(0, L_TO_H);

    // INT1 en flanco de subida
    ext_int_edge(1, L_TO_H);

    clear_interrupt(INT_EXT);
    clear_interrupt(INT_EXT1);

    enable_interrupts(INT_EXT);
    enable_interrupts(INT_EXT1);
    
}

void protolink_smt_init(void)
{


   // ---- SMT1 como contador de pulsos en FLOW1_PIN (INT0) ----
  
   setup_smt1(
           SMT_ENABLED 
           | SMT_TMR_ROLLSOVER          // 0, opcional, lo dejamos "por claridad"
           | SMT_SIG_ACTIVE_HIGH        // 0, señal activa en alto
           | SMT_TMR_INCREMENTS_ON_RE   // 0, flanco de subida
           | SMT_DIV_BY_1               // 0
           | SMT_REPEAT_DATA_ACQ_MODE   // 0x4000, adquisición continua
           | SMT_MODE_COUNTER           // 0x0800, modo contador
           | SMT_CLK_FOSC_DIV_4         // 0, vale para tenerlo explícito
           | SMT_START_NOW              // arranca inmediatamente
           );
   smt1_reset_timer();                     // contador = 0

   // ---- SMT2 como contador de pulsos en FLOW2_PIN (INT1) ----
   
   setup_smt2(
           SMT_ENABLED 
           | SMT_TMR_ROLLSOVER          // 0, opcional, lo dejamos "por claridad"
           | SMT_SIG_ACTIVE_HIGH        // 0, señal activa en alto
           | SMT_TMR_INCREMENTS_ON_RE   // 0, flanco de subida
           | SMT_DIV_BY_1               // 0
           | SMT_REPEAT_DATA_ACQ_MODE   // 0x4000, adquisición continua
           | SMT_MODE_COUNTER           // 0x0800, modo contador
           | SMT_CLK_FOSC_DIV_4         // 0, vale para tenerlo explícito
           | SMT_START_NOW              // arranca inmediatamente
           );
   smt2_reset_timer();
}
int1 protolink_one_second(void){
    if(Flag != lastFlag){
        lastFlag = flag;
        return true;
    }
    return false;
}
#endif
#define LOADER_END   0x5FF  //ajuste de memoria para protolink v2 bootloader
#endif	/* PROTOLINK_H */

