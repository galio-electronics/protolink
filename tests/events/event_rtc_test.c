#define main core_suite_main
#include "event_test.c"
#undef main
#define APP_EVENT_RTC 1
#define RTC_HAS_BATTERY 1
#define TRUE 1
#define FALSE 0
#include "mcp79402.h"
static uint8_t rtc_regs[32];
void delay_ms(unsigned n) { now_ms+=n; }
void rtc_bus_init(void) { }
bool rtc_bus_read(uint8_t reg,uint8_t *data,uint8_t n)
{ if(fail_bus) return false; memcpy(data,rtc_regs+reg,n); return true; }
bool rtc_bus_write(uint8_t reg,uint8_t *data,uint8_t n)
{
   if(fail_bus) return false;
   memcpy(rtc_regs+reg,data,n);
   if(rtc_regs[0]&128) rtc_regs[3]|=32; else rtc_regs[3]&=~32;
   return true;
}
#include "event_rtc.c"
#include "mcp79402.c"
int main(void)
{
   rtc_datetime_t dt={2024,2,29,12,0,0};
   ev_u8 state;
   unsigned i;
   fresh();
   assert(rtc_set_datetime(&dt));
   assert(rtc_get_datetime(&dt)); assert(rtc_get_datetime(&dt));
   event_manager_get_status(EM_RTC,&state); assert(state==EH_OK);
   emit(1); drain(); read_param(0);
   assert(event_get_le(event_storage_read_data+4,4)==1709208000UL);
   rtc_regs[3]|=16;
   for(i=0;i<3;i++) assert(!rtc_get_datetime(&dt));
   event_manager_get_status(EM_RTC,&state); assert(state==EH_DEGRADED);
   assert(!event_rtc_synced); drain();
   rtc_regs[3]&=~16; /* Clearing PWRFAIL alone does not assert UTC sync. */
   assert(rtc_get_datetime(&dt)); assert(!event_rtc_synced);
   assert(rtc_set_datetime(&dt));
   assert(rtc_get_datetime(&dt)); assert(rtc_get_datetime(&dt)); drain();
   event_manager_get_status(EM_RTC,&state); assert(state==EH_OK);
   fail_bus=1;
   for(i=0;i<3;i++) assert(!rtc_get_datetime(&dt));
   emit(2); ticks(100);
   event_manager_get_status(EM_RTC,&state); assert(state==EH_FAULT);
   assert(event_storage_state==STORAGE_ERROR);
   fail_bus=0; drain();
   dt.year=2023; dt.month=2; dt.day=29; assert(!rtc_validate(&dt));
   puts("PASS RTC: UTC epoch, sync, power failure, invalid date, recovery, shared RTC/EEPROM bus failure");
   return 0;
}
