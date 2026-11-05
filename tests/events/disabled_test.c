#include <assert.h>
#include <stdio.h>
#define EVENT_HOST 1
#include "event_variant_monitor.h"
#include "event_manager.h"
ev_u32 event_port_millis(void) { return 0; }
ev_bool event_port_read(ev_u16 a,ev_u8 *d) { assert(0); return 0; }
ev_bool event_port_write(ev_u16 a,ev_u8 *d) { assert(0); return 0; }
ev_bool event_port_ready(void) { assert(0); return 0; }
#include "event_crc.c"
#include "event_storage.c"
#include "event_manager.c"
int main(void)
{
   ev_u8 i,state;
   event_manager_init();
   for(i=1;i<EM_COUNT;i++) {
      assert(event_manager_get_status(i,&state) && state==EH_DISABLED);
      assert(!event_manager_report(i,EH_FAULT,0,0,0));
   }
   for(i=0;i<100;i++) event_manager_task();
   assert(event_storage_state==STORAGE_DISABLED && !event_queue_count);
   assert(event_manager_global_status()==EH_OK);
   puts("PASS absent modules: disabled Monitor software configuration, no bus access, no false alarms");
   return 0;
}
