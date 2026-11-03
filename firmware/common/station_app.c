/* Ejecutor cooperativo comun: incluido una sola vez por cada aplicacion CCS. */
#include "diagnostics.c"
struct {
   unsigned int8 inputs_raw;
} station_state;

void main(void)
{
   unsigned int32 now, last_inputs=0, last_led=0, last_100ms=0;
   unsigned int32 last_1000ms=0, last_status=0;
   int1 led_on=FALSE;
   disable_interrupts(GLOBAL);
   protolink_io_init();
   station_state.inputs_raw = protolink_read_inputs();
   protolink_timer2_init();
   diagnostics_init();
   enable_interrupts(GLOBAL);
   diagnostics_info();
   app_init();
   while(TRUE)
   {
#ifdef APP_FAST_TASK
      app_task(); // Avance breve de drivers cooperativos que requieren sub-ms.
#endif
      now = protolink_millis();
      diagnostics_task(station_state.inputs_raw);
      if((unsigned int32)(now-last_inputs) >= 10)
      {
         last_inputs = now;
         station_state.inputs_raw = protolink_read_inputs();
         app_task_10ms(station_state.inputs_raw);
      }
      if((unsigned int32)(now-last_led) >= 500)
      {
         last_led = now;
         led_on = !led_on;
         output_bit(LED1,led_on == ((STATION_LED_ACTIVE_MASK & 1) != 0));
      }
      if((unsigned int32)(now-last_100ms) >= 100)
      {
         last_100ms = now;
         app_task_100ms();
      }
      if((unsigned int32)(now-last_1000ms) >= 1000)
      {
         last_1000ms = now;
         app_task_1000ms();
      }
      if((unsigned int32)(now-last_status) >= APP_STATUS_INTERVAL_MS)
      {
         last_status = now;
         diagnostics_status(station_state.inputs_raw);
      }
   }
}
