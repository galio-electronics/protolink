#ifndef GALIO_STATION_LOGGER_H
#define GALIO_STATION_LOGGER_H
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARNING 2
#define LOG_ERROR 3
#define LOG_CRITICAL 4
extern unsigned int32 logger_lost, logger_written, logger_max_write_ms;
extern int1 logger_enabled;
void logger_init(void);
void logger_task(void);
void logger_flush(void);
int1 logger_event(unsigned int8 level, char *source, char *message);
int1 logger_event_code(unsigned int8 level, char *source, char *event, char *message);
int1 logger_telemetry(char *data);
#endif
