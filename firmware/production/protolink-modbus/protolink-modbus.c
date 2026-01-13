/*
 * Protolink Modbus v1.0.0
 * Galio Electronics
 * Nov 2025
 */

#include "../../protolink/v2.h"
#include <stdint.h>
#include <ieeefloat.c>
#include <stdlib.h>
#include <bootloader.h>


//MODBUS DEFINES
#define MODBUS_PROTOCOL             MODBUS_PROTOCOL_SERIAL
#define MODBUS_TYPE                 MODBUS_TYPE_SLAVE
#define MODBUS_SERIAL_TYPE          MODBUS_RTU     //use MODBUS_ASCII for ASCII mode
#define MODBUS_SERIAL_RX_BUFFER_SIZE 64
#define MODBUS_SERIAL_BAUD          115200
#define MODBUS_PARITY               "NONE"
//#define MODBUS_SERIAL_INT_SOURCE    MODBUS_INT_RDA4   //RS485
#define MODBUS_SERIAL_INT_SOURCE    MODBUS_INT_RDA   //ETHERNET
#define MODBUS_SERIAL_ENABLE_PIN    TX_ENABLE
#define MODBUS_SERIAL_RX_ENABLE     RX_ENABLE
#define MODBUS_TIMER_UPDATE         MODBUS_TIMER_ISR
#define MODBUS_TIMER_USED           MODBUS_TIMER_T2
#define MODBUS_SERIAL_RX_PIN        RX_4
#define MODBUS_SERIAL_TX_PIN        TX_4
#define MODBUS_SERIAL_TIMEOUT       100000  //solo master
#define MODBUS_ADDRESS 0x01

#define ADS1115_I2C_ADDR_7BIT  0x49
#use i2c(MASTER,FAST,SDA=PIN_SDA,SCL=PIN_SCL,STREAM=ADS_I2C )

#define DEVICE_DEBUG            TRUE
#define DEVICE_SERIAL_SOURCE    UART_INT_RDA2  //PORT SERIAL 1
#define DEVICE_SERIAL_BAUD      9600
#define DEVICE_RST_PIN          ETH_RST

#include "modbus_map.c"
#include "modbus_app.c"


// Numero de interrupciones de TIMER0 para ~1 segundo
// 2.048 ms * 488 ? 999.424 ms
int16 NInts = 488;

// VARIABLES GLOBALES
int16 C_Ints = 0;   // Contador de interrupciones ocurridas
int1 Flag = 0;     // Flag que cambia cada ~1 segundo

// Ejemplo de uso: hacer algo cada vez que cambie Flag (~1s)
int1 lastFlag = 0;


void led_toggle(void){
    if(Flag != lastFlag){
        lastFlag = flag;
        output_toggle(LED1);
    }
}

void i2c_scan_ads(void)
{
   int8 addr7;
   int8 addr8;
   int8 ack;

   protolink_debug_msg( "\r\n[I2C SCAN] Buscando ADS1115...\r\n");

   for(addr7 = 0x48; addr7 <= 0x4B; addr7++)   // solo rango típico ADS
   {
      addr8 = (addr7 << 1);   // write address

      i2c_start(ADS_I2C);
      ack = i2c_write(ADS_I2C, addr8);
      i2c_stop(ADS_I2C);

      if(ack == 0)
      {
         fprintf(DEBUG," -> Dispositivo responde en 7-bit 0x%X (8-bit write 0x%X)\r\n",
                addr7, addr8);
      }
   }
   protolink_debug_msg("[I2C SCAN] Fin.\r\n");
}

float adc_read_avg_mA(int16 n_samples)
{
   float acc = 0.0f;
   for(int16 i = 0; i < n_samples; i++)
   {
      acc += ina281_read_current_mA(&loop_ch0);  // tu canal
      delay_ms(5);
   }
   return acc / (float)n_samples;
}

#INT_TIMER0
void  TIMER0_isr(void) 
{
    // Incrementamos primero
   C_Ints++;

   if(C_Ints >= NInts)
   {
      C_Ints = 0;
      Flag = !Flag;   // Toggle cada ~1s
   }
   clear_interrupt(INT_TIMER0);
}

void init_timer0_1s(void)
{
   // Timer0 interno, 8 bits, prescaler 1:128
   // (Las etiquetas pueden ser T0_* o RTCC_* segun version CCS)
   setup_timer_0(T0_INTERNAL | T0_DIV_128 | T0_8_BIT);
 
   C_Ints = 0;
   Flag = 0;

   clear_interrupt(INT_TIMER0);
   enable_interrupts(INT_TIMER0);
   enable_interrupts(GLOBAL);
}

void main() {
    
    protolink_io_init(); //Inicializamos pinout 
    init_timer0_1s();
    set_timer0(0);

    output_high(ETH_RST);
    // Inicializar Modbus
    modbus_init();
    mb_map_init();
    protolink_debug_msg("\r\n----- TEST PROTOLINK V2 MODBUS -----\r\n");
    if(ads1115_ping())
    {
       protolink_debug_data("ADS1115 detectado en addr 0x%X (8-bit)\r\n", ADS1115_ADDR_WRITE);
    }
    else
    {
       protolink_debug_data("ERROR: ADS1115 NO responde en addr 0x%X (8-bit)\r\n", ADS1115_ADDR_WRITE);
    }
 
    //i2c_scan_ads();
    delay_ms(100); // estabilizar

    //int16 cfg = ads1115_read_reg(ADS1115_REG_CONFIG);

//    DEBUG_MSG("\r\nCALIBRACION\r\n");
//    delay_ms(1000);
//    float I_meas_low = adc_read_avg_mA(200);   // 200 muestras
//    DEBUG_DATA("I_meas_low: %.2f\r\n",I_meas_low);   
//    delay_ms(1000);
//    delay_ms(1);
//    float I_meas_high = adc_read_avg_mA(200);
//    DEBUG_DATA("I_meas_high: %.2f\r\n",I_meas_high);
//    delay_ms(1000);
//    delay_ms(1);
   // Loop principal
   while(TRUE) {
       //Atender Modbus (recibir y responder)
       mb_app_task();
       led_toggle(); 
   }
}

