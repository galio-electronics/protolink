#include "event_manager.h"
static ev_u8 ev_queue[EVENT_QUEUE_SIZE][32];
ev_u8 event_queue_count;
ev_u32 event_queue_lost;
static ev_u8 ev_health[EM_COUNT+EVENT_RS485_DEVICES-1];
static ev_u8 ev_candidate[EM_COUNT+EVENT_RS485_DEVICES-1];
static ev_u8 ev_consecutive[EM_COUNT+EVENT_RS485_DEVICES-1];
static ev_u8 ev_fail_limit[EM_COUNT],ev_good_limit[EM_COUNT];
static ev_u32 ev_uptime,ev_last_ms,ev_time_ms,ev_utc;
static ev_u16 ev_remainder;
static ev_bool ev_time_valid,ev_storage_fault,ev_crc_reported;
static void ev_clock(void)
{
   ev_u32 now=event_port_millis(),delta=now-ev_last_ms;
   ev_last_ms=now;
   ev_uptime+=delta/1000;
   ev_remainder+=(ev_u16)(delta%1000);
   if(ev_remainder>=1000) { ++ev_uptime; ev_remainder-=1000; }
}
void event_manager_time(ev_bool valid,ev_u32 utc)
{
   ev_time_valid=valid; ev_utc=utc; ev_time_ms=event_port_millis();
}
ev_bool event_manager_emit(ev_u8 module,ev_u16 code,ev_u8 previous,ev_u8 state,ev_u8 flags,ev_u32 p1,ev_u32 p2)
{
   ev_u8 i,j,victim=0,lowest=3;
   ev_u8 *r;
   ev_u32 age;
   if(module>=EM_COUNT || ev_health[module]==EH_DISABLED || (code>>8)!=module+1) return 0;
   ev_clock();
   if(event_queue_count==EVENT_QUEUE_SIZE) {
      for(i=0;i<EVENT_QUEUE_SIZE;i++) if((ev_queue[i][17]&3)<lowest) {
         lowest=ev_queue[i][17]&3; victim=i;
      }
      ++event_queue_lost;
      if((flags&3)<=lowest) return 0;
      for(i=victim;i<EVENT_QUEUE_SIZE-1;i++) for(j=0;j<32;j++) ev_queue[i][j]=ev_queue[i+1][j];
      --event_queue_count;
   }
   r=ev_queue[event_queue_count++];
   for(i=0;i<32;i++) r[i]=0;
   age=event_port_millis()-ev_time_ms;
   if(ev_time_valid && age<=EVENT_TIME_MAX_AGE_MS) event_put_le(r+4,ev_utc+age/1000,4);
   else flags |= EF_TIME_INVALID;
   event_put_le(r+8,ev_uptime,4); event_put_le(r+12,code,2);
   r[14]=module; r[15]=previous; r[16]=state; r[17]=flags;
   event_put_le(r+18,p1,4); event_put_le(r+22,p2,4);
   return 1;
}
void event_manager_init(void)
{
   ev_u8 i;
   event_queue_count=0; event_queue_lost=0;
   ev_uptime=0; ev_remainder=0; ev_last_ms=event_port_millis();
   ev_time_valid=0; ev_storage_fault=0; ev_crc_reported=0;
   for(i=0;i<EM_COUNT+EVENT_RS485_DEVICES-1;i++) {
      ev_health[i]=((EVENT_MODULE_MASK & ((ev_u16)1<<(i<EM_COUNT ? i : EM_RS485)))!=0) ? EH_UNKNOWN : EH_DISABLED;
      ev_candidate[i]=EH_UNKNOWN; ev_consecutive[i]=0;
   }
   for(i=0;i<EM_COUNT;i++) { ev_fail_limit[i]=EVENT_FAIL_THRESHOLD; ev_good_limit[i]=EVENT_OK_THRESHOLD; }
   ev_health[EM_SYSTEM]=EH_OK;
   event_storage_init();
}
ev_bool event_manager_thresholds(ev_u8 module,ev_u8 fail,ev_u8 good)
{
   if(module>=EM_COUNT || !fail || !good) return 0;
   ev_fail_limit[module]=fail; ev_good_limit[module]=good; return 1;
}
ev_bool event_manager_observe(ev_u8 module,ev_u8 connection,ev_u8 observed,ev_u16 reason,ev_u32 p1,ev_u32 p2,ev_bool terminal)
{
   ev_u8 cell,previous,limit,flags;
   ev_u16 code;
   if(module==EM_SYSTEM || module>=EM_COUNT || observed<EH_OK || observed>EH_FAULT) return 0;
   if(module==EM_RS485) {
      if(connection>=EVENT_RS485_DEVICES) return 0;
      cell=connection ? EM_COUNT+connection-1 : EM_RS485;
   } else { if(connection) return 0; cell=module; }
   if(ev_health[cell]==EH_DISABLED) return 0;
   if(observed==ev_health[cell]) { ev_consecutive[cell]=0; ev_candidate[cell]=observed; return 1; }
   if(ev_candidate[cell]!=observed) { ev_candidate[cell]=observed; ev_consecutive[cell]=0; }
   if(ev_consecutive[cell]<255) ++ev_consecutive[cell];
   limit=observed==EH_OK ? ev_good_limit[module] : ev_fail_limit[module];
   if(!(terminal && module==EM_SD) && ev_consecutive[cell]<limit) return 1;
   previous=ev_health[cell]; ev_health[cell]=observed; ev_consecutive[cell]=0;
   if(previous==EH_UNKNOWN && observed==EH_OK) return 1;
   code=((ev_u16)(module+1)<<8) | (observed==EH_OK ? 2 : 1);
   if(module==EM_POWER && observed==EH_OK) code=EV_POWER_RECOVERED;
   if(observed!=EH_OK && reason) {
      if((module==EM_RTC && reason==EV_RTC_TIME_INVALID) ||
         (module==EM_POWER && (reason==EV_POWER_LOW || reason==EV_POWER_HIGH))) code=reason;
   }
   flags=observed==EH_OK ? ES_INFO : (observed==EH_DEGRADED ? ES_WARNING : ES_ERROR);
   if(module==EM_RS485) flags |= EF_EXTERNAL;
   return event_manager_emit(module,code,previous,observed,flags,p1,p2);
}
ev_bool event_manager_report(ev_u8 module,ev_u8 observed,ev_u16 reason,ev_u32 p1,ev_u32 p2)
{ return event_manager_observe(module,0,observed,reason,p1,p2,0); }
ev_bool event_manager_get_status(ev_u8 module,ev_u8 *state)
{
   ev_u8 i;
   ev_bool unknown;
   if(module>=EM_COUNT) return 0;
   *state=ev_health[module];
   if(module==EM_RS485 && *state!=EH_DISABLED) {
      unknown=(*state==EH_UNKNOWN);
      for(i=EM_COUNT;i<EM_COUNT+EVENT_RS485_DEVICES-1;i++) {
         if(ev_health[i]==EH_UNKNOWN) unknown=1;
         if(ev_health[i]>*state && ev_health[i]!=EH_DISABLED) *state=ev_health[i];
      }
      if(*state==EH_OK && unknown) *state=EH_UNKNOWN;
   }
   return 1;
}
ev_u8 event_manager_global_status(void)
{
   ev_u8 i,state,worst=EH_OK;
   ev_bool unknown=0;
   for(i=0;i<EM_COUNT;i++) {
      event_manager_get_status(i,&state);
      if(state==EH_UNKNOWN) unknown=1;
      if(state!=EH_DISABLED && state>worst) worst=state;
   }
   return worst==EH_OK && unknown ? EH_UNKNOWN : worst;
}
ev_u16 event_manager_count(void) { return event_storage_count; }
ev_bool event_manager_is_busy(void) { return event_storage_state!=STORAGE_READY || event_queue_count!=0; }
ev_bool event_manager_read(ev_u16 index) { return event_storage_read(index); }
void event_manager_task(void)
{
   ev_u8 i,j;
   ev_clock(); event_storage_task();
   if(event_storage_state==STORAGE_ERROR && !ev_storage_fault) {
      ev_storage_fault=1; ev_health[EM_EEPROM]=EH_FAULT;
      event_manager_emit(EM_EEPROM,EV_EEPROM_FAULT,EH_UNKNOWN,EH_FAULT,ES_ERROR,event_storage_errors,0);
   }
   if(event_storage_state==STORAGE_READY) {
      if(ev_storage_fault) {
         ev_storage_fault=0; ev_health[EM_EEPROM]=EH_OK;
         event_manager_emit(EM_EEPROM,EV_EEPROM_RECOVERED,EH_FAULT,EH_OK,ES_INFO,event_storage_errors,0);
      } else if(ev_health[EM_EEPROM]!=EH_DISABLED) ev_health[EM_EEPROM]=EH_OK;
      if(!ev_crc_reported) {
         ev_crc_reported=1;
         if(event_storage_corrupt) event_manager_emit(EM_EEPROM,EV_EEPROM_CRC_ERROR,EH_UNKNOWN,EH_DEGRADED,ES_WARNING,event_storage_corrupt,0);
      }
      if(event_queue_count && event_storage_submit(ev_queue[0])) {
         --event_queue_count;
         for(i=0;i<event_queue_count;i++) for(j=0;j<32;j++) ev_queue[i][j]=ev_queue[i+1][j];
      }
   }
}
/* Sticky flags: classify only a single unambiguous source. POR accepts BOR unknown. */
ev_u16 event_reset_code(ev_u8 pcon)
{
   if(pcon==0x3C || pcon==0x3D) return 0; /* POR represented by SYSTEM_BOOT */
   if(pcon==0x3E) return EV_RESET_BROWNOUT;
   if(pcon==0x2F || pcon==0x1F) return EV_RESET_WATCHDOG;
   if(pcon==0x37) return EV_RESET_EXTERNAL;
   if(pcon==0x3B) return EV_RESET_SOFTWARE;
   if(pcon==0xBF || pcon==0x7F) return EV_RESET_STACK_FAULT;
   return EV_RESET_UNKNOWN;
}
