/* Compile-only CCS fixture, not a Monitor firmware image or board port. */
#include <18F67K40.h>
#use delay(clock=64000000)
#define LOADER_END 0x5FF
#include <bootloader.h>
#include "../../firmware/common/event_variant_monitor.h"
#include "../../firmware/common/event_manager.h"
static ev_u32 fixture_tick;
ev_u32 event_port_millis(void) { return fixture_tick; }
ev_bool event_port_read(ev_u16 address,ev_u8 *data) { return 0; }
ev_bool event_port_write(ev_u16 address,ev_u8 *data) { return 0; }
ev_bool event_port_ready(void) { return 0; }
#include "../../firmware/common/event_crc.c"
#include "../../firmware/common/event_storage.c"
#include "../../firmware/common/event_manager.c"
void main(void)
{
   ev_u8 state;
   event_manager_init();
   event_manager_emit(EM_SYSTEM,EV_SYSTEM_BOOT,EH_UNKNOWN,EH_OK,ES_INFO,0,0);
   event_manager_report(EM_SD,EH_FAULT,0,0,0);
   event_manager_get_status(EM_SD,&state);
   while(TRUE) { ++fixture_tick; event_manager_task(); }
}
