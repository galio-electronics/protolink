#include "event_storage.h"
ev_u8 event_storage_state;
ev_u16 event_storage_count,event_storage_boot;
ev_u32 event_storage_errors,event_storage_corrupt,event_scan_ms;
ev_bool event_storage_read_done,event_storage_read_ok;
ev_u8 event_storage_read_data[32];
static ev_u8 ev_valid[192],ev_write[32],ev_scratch[32];
static ev_u16 ev_scan,ev_next,ev_latest,ev_read_slot,ev_read_left,ev_read_seen;
static ev_u32 ev_sequence,ev_started,ev_wait_started,ev_retry_started;
static ev_u8 ev_resume,ev_attempts;
static ev_bool ev_found,ev_clearing,ev_paused;
static ev_bool ev_slot_valid(ev_u16 slot) { return (ev_valid[slot/8] & (1<<(slot%8)))!=0; }
static void ev_slot_set(ev_u16 slot,ev_bool valid)
{
   ev_bool old=ev_slot_valid(slot);
   if(valid) ev_valid[slot/8] |= 1<<(slot%8);
   else ev_valid[slot/8] &= ~(1<<(slot%8));
   if(valid && !old) ++event_storage_count;
   if(!valid && old) --event_storage_count;
}
static ev_u16 ev_address(ev_u16 slot) { return EVENT_BASE+slot*32; }
static ev_u16 ev_advance(ev_u16 slot) { return slot==EVENT_SLOT_COUNT-1 ? 0 : slot+1; }
static void ev_storage_fail(ev_u8 resume)
{
   ++event_storage_errors;
   ev_resume=resume; ev_retry_started=event_port_millis(); ev_paused=1;
   if(++ev_attempts>=EVENT_RETRIES) event_storage_state=STORAGE_ERROR;
   else event_storage_state=resume;
}
void event_storage_init(void)
{
   ev_u16 i;
   for(i=0;i<192;i++) ev_valid[i]=0;
   event_storage_count=0; event_storage_boot=1;
   event_storage_errors=0; event_storage_corrupt=0; event_scan_ms=0;
   ev_scan=0; ev_next=0; ev_sequence=0; ev_found=0; ev_clearing=0;
   ev_attempts=0; ev_paused=0; event_storage_read_done=0;
   ev_started=event_port_millis();
   event_storage_state=EVENT_STORAGE_ENABLED ? STORAGE_SCANNING : STORAGE_DISABLED;
}
ev_bool event_storage_submit(ev_u8 *record)
{
   ev_u8 i;
   if(event_storage_state!=STORAGE_READY || ev_paused) return 0;
   for(i=0;i<32;i++) ev_write[i]=record[i];
   event_put_le(ev_write,ev_sequence,4);
   event_put_le(ev_write+26,event_storage_boot,2);
   event_put_le(ev_write+30,event_crc16(ev_write,30),2);
   event_storage_state=STORAGE_PREPARE; return 1;
}
ev_bool event_storage_read(ev_u16 index)
{
   if(event_storage_state!=STORAGE_READY || index>=event_storage_count) return 0;
   ev_read_slot=ev_next; ev_read_left=index; ev_read_seen=0;
   event_storage_read_done=0; event_storage_read_ok=0;
   event_storage_state=STORAGE_READING; return 1;
}
ev_bool event_storage_clear(void)
{
   ev_u8 i;
   if(event_storage_state!=STORAGE_READY) return 0;
   for(i=0;i<32;i++) ev_write[i]=255;
   ev_clearing=1; ev_next=0; event_storage_state=STORAGE_PREPARE; return 1;
}
void event_storage_task(void)
{
   ev_u8 i;
   ev_u32 seq,now=event_port_millis();
   ev_bool equal,erased;
   if(event_storage_state==STORAGE_ERROR) {
      if((ev_u32)(now-ev_retry_started)<EVENT_RECOVERY_MS) return;
      ev_attempts=0; ev_paused=0; event_storage_state=ev_resume;
      return;
   }
   if(ev_paused) {
      if((ev_u32)(now-ev_retry_started)<EVENT_RETRY_MS) return;
      ev_paused=0;
   }
   switch(event_storage_state) {
   case STORAGE_SCANNING:
      if(!event_port_read(ev_address(ev_scan),ev_scratch)) { ev_storage_fail(STORAGE_SCANNING); break; }
      ev_attempts=0;
      if(event_record_valid(ev_scratch)) {
         ev_slot_set(ev_scan,1); seq=event_get_le(ev_scratch,4);
         if(!ev_found || event_sequence_after(seq,ev_sequence)) {
            ev_found=1; ev_sequence=seq; ev_latest=ev_scan;
            event_storage_boot=(ev_u16)(event_get_le(ev_scratch+26,2)+1);
         }
      } else {
         erased=1;
         for(i=0;i<32;i++) if(ev_scratch[i]!=255) erased=0;
         if(!erased) ++event_storage_corrupt;
      }
      if(++ev_scan==EVENT_SLOT_COUNT) {
         if(ev_found) { ev_next=ev_advance(ev_latest); ++ev_sequence; }
         event_scan_ms=now-ev_started; event_storage_state=STORAGE_READY;
      }
      break;
   case STORAGE_PREPARE:
      /* No persistent metadata; invalidate RAM membership before overwrite. */
      ev_slot_set(ev_next,0); event_storage_state=STORAGE_WRITE; break;
   case STORAGE_WRITE:
      if(!event_port_write(ev_address(ev_next),ev_write)) { ev_storage_fail(STORAGE_WRITE); break; }
      ev_wait_started=now; event_storage_state=STORAGE_WAIT_READY; break;
   case STORAGE_WAIT_READY:
      if(now==ev_wait_started) break;
      if(event_port_ready()) event_storage_state=STORAGE_VERIFY;
      else if((ev_u32)(now-ev_wait_started)>=EVENT_WRITE_TIMEOUT_MS) ev_storage_fail(STORAGE_WRITE);
      break;
   case STORAGE_VERIFY:
      if(!event_port_read(ev_address(ev_next),ev_scratch)) { ev_storage_fail(STORAGE_WRITE); break; }
      equal=1; for(i=0;i<32;i++) if(ev_write[i]!=ev_scratch[i]) equal=0;
      if(!equal) { ev_storage_fail(STORAGE_WRITE); break; }
      ev_attempts=0;
      if(!ev_clearing) { ev_slot_set(ev_next,1); ++ev_sequence; }
      ev_next=ev_advance(ev_next);
      if(ev_clearing && ev_next) event_storage_state=STORAGE_PREPARE;
      else { ev_clearing=0; event_storage_state=STORAGE_READY; }
      break;
   case STORAGE_READING:
      if(++ev_read_seen>EVENT_SLOT_COUNT) {
         event_storage_read_done=1; event_storage_state=STORAGE_READY; break;
      }
      if(ev_slot_valid(ev_read_slot)) {
         if(!ev_read_left) {
            event_storage_read_ok=event_port_read(ev_address(ev_read_slot),event_storage_read_data);
            if(event_storage_read_ok) event_storage_read_ok=event_record_valid(event_storage_read_data);
            event_storage_read_done=1; event_storage_state=STORAGE_READY; break;
         }
         --ev_read_left;
      }
      ev_read_slot=ev_advance(ev_read_slot); break;
   }
}
