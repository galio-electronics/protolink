/*Project: TunaCore- BOOT
 * Description: Bootloader para tunacore y tunacoremini
 * Date: Julio 2021
 * Development: Tuna IoT Solution
 * www.tunaiot.com
 * 
 */


#include <18F67K40.h>
#device ADC = 10
#fuses HS
#FUSES NOPPS1WAY

#define COMPILATION_DATE    __DATE__
#define COMPILATION_TIME    __TIME__
#define HW_NAME     "ProtoLink"
#define BOOT_VER    "BOOT:1.2"


//LEDS y BOTON
#define LED1    PIN_F0
#define LED2    PIN_F1
#define USER    PIN_B2
//USB PINs
#define TX_5    PIN_E2
#define RX_5    PIN_E3

//PORT2 R232
#define TX_3    PIN_E0
#define RX_3    PIN_E1

//UART1
#pin_select U5TX=TX_5
#pin_select U5RX=RX_5

//UART3
#pin_select U3TX=TX_3
#pin_select U3RX=RX_3

//#use delay(crystal=20000000)
#use delay(crystal=16mhz,clock=64mhz)
#use rs232(baud=115200,parity=N,UART3,bits=8)

#define LOADER_END   0x5FF
#define _bootloader
// NOTE - User must include bootloader.h in application program
#include <bootloader.h>
#include "Protolink-LOADER.c"



#INT_GLOBAL
void isr(void){
	jump_to_isr(LOADER_END+9);
}

#org LOADER_END+2, LOADER_END+4
void application(void) {
  while(TRUE);
}

void main()
{
    output_low(LED1);
    output_low(LED2);
    

	// Enter Bootloader if Pin USER is high after a RESET
	if(input(USER))
	{
		output_high(LED1);
        output_high(LED2);
        printf("name=%s,version=%s,date=%s,time=%s\n",HW_NAME,BOOT_VER,COMPILATION_DATE,COMPILATION_TIME);
        delay_ms(1000);
        load_program();
	}

	application();

}
