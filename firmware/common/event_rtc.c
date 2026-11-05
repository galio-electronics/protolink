/* Include after mcp79402.h. Sync is explicit and is not inferred from ACK. */
static ev_bool event_rtc_synced=0;
void event_health_rtc(ev_bool success,rtc_datetime_t *dt)
{
   ev_u32 days=0,seconds;
   ev_u16 year;
   ev_u8 month,md;
   ev_bool trusted=success && event_rtc_synced;
   if(!success) event_rtc_synced=0;
   if(trusted) {
      for(year=1970;year<dt->year;year++) days+=(year%4==0) ? 366 : 365;
      for(month=1;month<dt->month;month++) {
         md=31;
         if(month==4 || month==6 || month==9 || month==11) md=30;
         if(month==2) md=(dt->year%4==0) ? 29 : 28;
         days+=md;
      }
      days+=dt->day-1;
      seconds=days*86400UL+(ev_u32)dt->hour*3600UL+(ev_u32)dt->minute*60UL+dt->second;
      event_manager_time(1,seconds);
   } else event_manager_time(0,0);
   event_manager_report(EM_RTC,trusted ? EH_OK : (rtc_error==RTC_BUS_ERROR ? EH_FAULT : EH_DEGRADED),
      trusted || rtc_error==RTC_BUS_ERROR ? 0 : EV_RTC_TIME_INVALID,rtc_error,event_rtc_synced);
}
