/*
 * File:   protolink-hello-world.c
 * Galio Electronics - Protolink Station 0.1.0
 */
#define PROTOLINK_DEFAULT 1
#include "../../station.h"
#include <bootloader.h>

// Unico escritor: tareas de main. Entradas crudas, sin asumir polaridad.
struct {
   unsigned int8 inputs_raw;
} station_state;

void main(void)
{
   unsigned int32 now;
   unsigned int32 last_inputs = 0;
   unsigned int32 last_led = 0;
   unsigned int32 last_100ms = 0;
   unsigned int32 last_1000ms = 0;
   int1 led_on = FALSE;

   disable_interrupts(GLOBAL);
   protolink_io_init();
   station_state.inputs_raw = protolink_read_inputs();
   protolink_timer2_init();
   enable_interrupts(GLOBAL);

   while(TRUE)
   {
      now = protolink_millis();
      // Resta unsigned: correcta tambien al desbordarse el contador.
      // Sin rafagas de recuperacion si una futura tarea tarda demasiado.
      if((unsigned int32)(now - last_inputs) >= 10)
      {
         last_inputs = now;
         station_state.inputs_raw = protolink_read_inputs();
      }
      if((unsigned int32)(now - last_led) >= 500)
      {
         last_led = now;
         led_on = !led_on;
         output_bit(LED1,led_on == ((STATION_LED_ACTIVE_MASK & (1 << 0)) != 0));
      }
      if((unsigned int32)(now - last_100ms) >= 100)
      {
         last_100ms = now;
         // Punto de extension: tareas no bloqueantes de 100 ms.
      }
      if((unsigned int32)(now - last_1000ms) >= 1000)
      {
         last_1000ms = now;
         // Punto de extension: supervision no bloqueante de 1000 ms.
      }
   }
}
