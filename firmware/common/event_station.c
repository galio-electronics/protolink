/* Include after station.h, before app code. No ISR invokes this library. */
#include "event_manager.h"
extern unsigned int16 diagnostics_reset_cause;
#if (EVENT_STORAGE_ENABLED == 1)
#include "station_i2c.c"
#endif
#byte event_pcon0=getenv("SFR:PCON0")
ev_u8 event_reset_raw;
void event_station_capture(void) { event_reset_raw=event_pcon0; }
ev_u32 event_port_millis(void) { return protolink_millis(); }
ev_bool event_port_read(ev_u16 address,ev_u8 *data)
{
#if (EVENT_STORAGE_ENABLED == 1)
   if(address<EVENT_BASE || (address & 31)) return 0;
   return station_i2c_read(0xA0,address,2,data,32);
#else
   return 0;
#endif
}
ev_bool event_port_write(ev_u16 address,ev_u8 *data)
{
#if (EVENT_STORAGE_ENABLED == 1)
   if(address<EVENT_BASE || (address & 31)) return 0;
   return station_i2c_write(0xA0,address,2,data,32);
#else
   return 0;
#endif
}
ev_bool event_port_ready(void)
{
#if (EVENT_STORAGE_ENABLED == 1)
   return station_i2c_probe(0xA0);
#else
   return 0;
#endif
}
#include "event_crc.c"
#include "event_storage.c"
#include "event_manager.c"
void event_station_init(void)
{
   ev_u16 code;
#if (EVENT_STORAGE_ENABLED == 1)
   rtc_bus_init();
#endif
   event_manager_init();
   event_manager_emit(EM_SYSTEM,EV_SYSTEM_BOOT,EH_UNKNOWN,EH_OK,ES_INFO|EF_RESET_LATE,event_reset_raw,diagnostics_reset_cause);
   code=event_reset_code(event_reset_raw);
   if(code) event_manager_emit(EM_SYSTEM,code,EH_UNKNOWN,EH_OK,ES_WARNING|EF_RESET_LATE,event_reset_raw,diagnostics_reset_cause);
}
