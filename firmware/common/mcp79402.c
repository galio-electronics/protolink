#include "mcp79402.h"
/* Board supplies rtc_bus_init/read/write with ACK checking and timeouts. */
unsigned int8 rtc_error = RTC_OK;
static unsigned int8 rtc_bcd(unsigned int8 n) { return ((n/10)<<4)|(n%10); }
static unsigned int8 rtc_decimal(unsigned int8 n)
{
   if((n & 15)>9 || (n>>4)>9) return 255;
   return (n>>4)*10+(n & 15);
}
int1 rtc_validate(rtc_datetime_t *dt)
{
   unsigned int8 days = 31;
   if(dt->year<2000 || dt->year>2099 || dt->month<1 || dt->month>12 ||
      dt->hour>23 || dt->minute>59 || dt->second>59) return FALSE;
   if(dt->month==4 || dt->month==6 || dt->month==9 || dt->month==11) days=30;
   if(dt->month==2) days = (dt->year%4 == 0) ? 29 : 28;
   return dt->day>=1 && dt->day<=days;
}
static int1 rtc_read(unsigned int8 reg, unsigned int8 *data, unsigned int8 n)
{
   if(rtc_bus_read(reg,data,n)) return TRUE;
   rtc_error = RTC_BUS_ERROR; return FALSE;
}
static int1 rtc_write(unsigned int8 reg, unsigned int8 *data, unsigned int8 n)
{
   if(rtc_bus_write(reg,data,n)) return TRUE;
   rtc_error = RTC_BUS_ERROR; return FALSE;
}
int1 rtc_init(void)
{
   unsigned int8 weekday;
   rtc_error = RTC_OK;
   rtc_bus_init();
   if(!rtc_read(3,&weekday,1)) return FALSE;
   /* Preserve PWRFAIL and timestamps; never start a stopped clock blindly. */
   if(RTC_HAS_BATTERY && !(weekday & 8)) {
      weekday |= 8;
      if(!rtc_write(3,&weekday,1)) return FALSE;
   }
   return TRUE;
}
int1 rtc_get_datetime(rtc_datetime_t *dt)
{
   unsigned int8 r[7], sec, attempt;
   rtc_error = RTC_OK;
   /* Retry a burst crossing a second boundary. */
   for(attempt=0;attempt<3;attempt++) {
      if(!rtc_read(0,r,7) || !rtc_read(0,&sec,1)) return FALSE;
      if(sec == r[0]) break;
   }
   if(attempt == 3) { rtc_error = RTC_INVALID; return FALSE; }
   if(!(r[0]&0x80) || !(r[3]&0x20)) { rtc_error=RTC_STOPPED; return FALSE; }
   if(r[2]&0x40) { rtc_error=RTC_INVALID; return FALSE; }
   dt->second=rtc_decimal(r[0]&0x7F); dt->minute=rtc_decimal(r[1]&0x7F);
   dt->hour=rtc_decimal(r[2]&0x3F); dt->day=rtc_decimal(r[4]&0x3F);
   dt->month=rtc_decimal(r[5]&0x1F);
   /* CCS 5.081 miscompiles widening arithmetic directly on this byte return. */
   sec=rtc_decimal(r[6]);
   dt->year=2000;
   dt->year+=sec;
   if(!rtc_validate(dt)) { rtc_error=RTC_INVALID; return FALSE; }
   /* Conservative: operator must inspect/acknowledge a power interruption. */
   if(r[3]&0x10) { rtc_error=RTC_POWER_FAIL; return FALSE; }
   return TRUE;
}
int1 rtc_is_valid(void) { rtc_datetime_t dt; return rtc_get_datetime(&dt); }
int1 rtc_get_power_fail_status(void)
{
   unsigned int8 v;
   rtc_error=RTC_OK;
   if(!rtc_read(3,&v,1)) return FALSE;
   return (v & 0x10)!=0;
}
int1 rtc_get_power_timestamps(unsigned int8 *raw8) { return rtc_read(0x18,raw8,8); }
int1 rtc_clear_power_fail_status(void)
{
   unsigned int8 v;
   rtc_error=RTC_OK;
   if(!rtc_read(3,&v,1)) return FALSE;
   v &= ~0x10;
   return rtc_write(3,&v,1);
}
int1 rtc_enable_sqw(void)
{
   unsigned int8 v;
   if(!rtc_read(7,&v,1)) return FALSE;
   v = (v & 0x80) | 0x40; // Crystal, 1 Hz, disable alarms/coarse trim.
   return rtc_write(7,&v,1);
}
int1 rtc_set_datetime(rtc_datetime_t *dt)
{
   unsigned int8 r[7], weekday;
   unsigned int16 days, y, attempts;
   unsigned int8 m, md;
   rtc_error=RTC_OK;
   if(!rtc_validate(dt)) { rtc_error=RTC_INVALID; return FALSE; }
   if(!rtc_read(3,&weekday,1)) return FALSE;
   r[0]=0; // Stop oscillator before changing calendar.
   if(!rtc_write(0,r,1)) return FALSE;
   for(attempts=0;attempts<500;attempts++) {
      if(!rtc_read(3,&r[3],1)) return FALSE;
      if(!(r[3]&0x20)) break;
      delay_ms(1);
   }
   if(attempts==500) { rtc_error=RTC_STOPPED; return FALSE; }
   days=dt->day-1;
   for(y=2000;y<dt->year;y++) {
      if(y%4==0) days+=366; else days+=365;
   }
   for(m=1;m<dt->month;m++) {
      md=31;
      if(m==4 || m==6 || m==9 || m==11) md=30;
      if(m==2) md=dt->year%4 == 0 ? 29 : 28;
      days += md;
   }
   r[0]=rtc_bcd(dt->second); r[1]=rtc_bcd(dt->minute); r[2]=rtc_bcd(dt->hour);
   r[3]=((days+6)%7)+1; // Sunday=1; 2000-01-01 Saturday.
   r[3] |= (weekday & 0x18); // Preserve failure evidence and backup setting.
#if (RTC_HAS_BATTERY == 1)
   r[3] |= 8;
#endif
   r[4]=rtc_bcd(dt->day); r[5]=rtc_bcd(dt->month); r[6]=rtc_bcd(dt->year-2000);
   if(!rtc_write(0,r,7)) return FALSE;
   r[0] |= 0x80;
   if(!rtc_write(0,r,1)) return FALSE;
   for(attempts=0;attempts<1000;attempts++) {
      delay_ms(2);
      if(!rtc_read(3,&weekday,1)) return FALSE;
      if(weekday & 0x20) return TRUE;
   }
   rtc_error=RTC_STOPPED; return FALSE;
}
