#ifndef PROTOLINK_BUZZER_H
#define PROTOLINK_BUZZER_H

// Active buzzer. Define pin and polarity before including buzzer.c once.
// Requires protolink_millis(); call from main only, with Timer2 initialized.
#ifndef BUZZER_PIN
#error Define BUZZER_PIN in app_config.h
#endif
#ifndef BUZZER_ACTIVE_LEVEL
#define BUZZER_ACTIVE_LEVEL 1
#endif

void buzzer_init(void);
// A new request restarts the duration; zero stops immediately.
void buzzer_beep(unsigned int16 duration_ms);
void buzzer_stop(void);
// Call frequently; actual shutoff may be delayed by main-loop latency.
void buzzer_task(void);
#endif
