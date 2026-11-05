/* Demo Station: LED1 -> LED2 -> LED3 -> LED4, repetidamente. */
#define PROTOLINK_DEFAULT 1
#include "../../../station.h"
#include <bootloader.h>

#define BLINK_INTERVAL_MS 250

// Respeta la polaridad de cada LED definida por la placa.
void blink_show(unsigned int8 led, int1 on)
{
   output_bit(LED1, (on && led == 0) == ((STATION_LED_ACTIVE_MASK & 0x01) != 0));
   output_bit(LED2, (on && led == 1) == ((STATION_LED_ACTIVE_MASK & 0x02) != 0));
   output_bit(LED3, (on && led == 2) == ((STATION_LED_ACTIVE_MASK & 0x04) != 0));
   output_bit(LED4, (on && led == 3) == ((STATION_LED_ACTIVE_MASK & 0x08) != 0));
}

void main(void)
{
   unsigned int32 now, last_change;
   unsigned int8 led = 0;
   int1 on = TRUE;

   disable_interrupts(GLOBAL);
   protolink_io_init();
   protolink_timer2_init();
   enable_interrupts(GLOBAL);
   blink_show(led, on);
   last_change = protolink_millis();

   while(TRUE)
   {
      now = protolink_millis();
      if((unsigned int32)(now - last_change) >= BLINK_INTERVAL_MS)
      {
         last_change = now;
         on = !on;
         if(on)
            led = (led + 1) & 0x03;
         blink_show(led, on);
      }
   }
}
