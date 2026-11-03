/*Project: Protolink-Station-Monitor-BOOT
 * Description: Bootloader compartido para Station y Monitor
 * Development: Galio Electronics
 * www.galio.dev
 */

// Debe preceder a station.h: incluye el driver CCS de memoria.
#define _bootloader
// Solo se reutiliza el hardware comun; no activar PROTOLINK_DEFAULT.
#include "../../station.h"

#define COMPILATION_DATE __DATE__
#define COMPILATION_TIME __TIME__
#define HW_NAME "Protolink-Station-Monitor"
#define BOOT_VER "BOOT:1.2"

// Confirmado: el boton presionado lleva RA4 a nivel bajo.
#ifndef BOOT_ACTIVE_LEVEL
#define BOOT_ACTIVE_LEVEL 0
#endif

#pin_select U1TX=TX_1
#pin_select U1RX=RX_1
#use rs232(baud=115200,parity=N,UART1,bits=8,errors)

#include <bootloader.h>
#include "Protolink-LOADER.c"

#INT_GLOBAL
void isr(void)
{
   jump_to_isr(LOADER_END+9);
}

#org LOADER_END+2, LOADER_END+4
void application(void)
{
   while(TRUE);
}

void main(void)
{
   disable_interrupts(GLOBAL);
   // DO inactivas antes de comprobar BOOT; RA4 queda como entrada digital.
   protolink_io_init();
   // station.h usa fast_io: UART1 requiere configurar TRIS explicitamente.
   output_high(TX_1);
   set_tris_c(0b10111111);

   // Se evalua al arrancar, como en los bootloaders existentes.
   if(input(MCU_BOOT) == BOOT_ACTIVE_LEVEL)
   {
      output_high(LED1);
      output_high(LED2);
      printf("name=%s,version=%s,date=%s,time=%s\n",
             HW_NAME,BOOT_VER,COMPILATION_DATE,COMPILATION_TIME);
      delay_ms(1000);
      load_program();
   }

   application();
}