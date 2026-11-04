#ifndef GALIO_MCP79402_H
#define GALIO_MCP79402_H
typedef struct {
   unsigned int16 year;
   unsigned int8 month, day, hour, minute, second;
} rtc_datetime_t;
#define RTC_OK 0
#define RTC_BUS_ERROR 1
#define RTC_INVALID 2
#define RTC_STOPPED 3
#define RTC_POWER_FAIL 4
extern unsigned int8 rtc_error;
int1 rtc_init(void);
int1 rtc_validate(rtc_datetime_t *dt);
int1 rtc_get_datetime(rtc_datetime_t *dt);
int1 rtc_set_datetime(rtc_datetime_t *dt);
int1 rtc_is_valid(void);
int1 rtc_get_power_fail_status(void);
int1 rtc_clear_power_fail_status(void);
int1 rtc_get_power_timestamps(unsigned int8 *raw8);
int1 rtc_enable_sqw(void);
#endif
