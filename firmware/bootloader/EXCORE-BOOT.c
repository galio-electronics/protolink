/*Project: Protolink-EXCORE-BOOT
 * Description: Protolink EXCORE Bootloader
 * Date: December 2025
 * Development: Galio Electronics
 * www.galio.dev
 * 
 */


#include "../protolink/excore.h"
#define LED2 PIN_D1 //se agrega como compatibilidad al loader
#define COMPILATION_DATE    __DATE__
#define COMPILATION_TIME    __TIME__
#define HW_NAME     "EXCORE-V1"
#define BOOT_VER    "BOOT:1.0"
#use delay(crystal=16mhz,clock=64mhz)
#use rs232(baud=115200,parity=N,UART5,bits=8)
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
    

	// Enter Bootloader if Pin USER is LOW after a RESET
	if(!input(USER))
	{
		output_high(LED1);
        output_high(LED2);
        printf("name=%s,version=%s,date=%s,time=%s\n",HW_NAME,BOOT_VER,COMPILATION_DATE,COMPILATION_TIME);
        delay_ms(1000);
        load_program();
	}

	application();

}
