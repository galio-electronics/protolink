#include "buzzer.h"

static int1 buzzer_running=FALSE;
static unsigned int32 buzzer_started_ms;
static unsigned int16 buzzer_duration_ms;

void buzzer_stop(void)
{
   output_bit(BUZZER_PIN,!BUZZER_ACTIVE_LEVEL);
   buzzer_running=FALSE;
}

void buzzer_init(void)
{
   buzzer_stop(); // Preload inactive latch before enabling the output.
   output_drive(BUZZER_PIN); // Explicit direction for fast_io; only this pin.
}

void buzzer_beep(unsigned int16 duration_ms)
{
   if(!duration_ms) { buzzer_stop(); return; }
   buzzer_started_ms=protolink_millis();
   buzzer_duration_ms=duration_ms;
   buzzer_running=TRUE;
   output_bit(BUZZER_PIN,BUZZER_ACTIVE_LEVEL);
}

void buzzer_task(void)
{
   if(buzzer_running &&
      (unsigned int32)(protolink_millis()-buzzer_started_ms)>=buzzer_duration_ms)
      buzzer_stop();
}
