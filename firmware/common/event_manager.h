#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H
#include "event_codes.h"
#include "event_storage.h"
extern ev_u8 event_queue_count;
extern ev_u32 event_queue_lost;
void event_manager_init(void);
void event_manager_task(void);
ev_bool event_manager_report(ev_u8 module,ev_u8 observed,ev_u16 reason,ev_u32 p1,ev_u32 p2);
ev_bool event_manager_observe(ev_u8 module,ev_u8 connection,ev_u8 observed,ev_u16 reason,ev_u32 p1,ev_u32 p2,ev_bool terminal);
ev_bool event_manager_emit(ev_u8 module,ev_u16 code,ev_u8 previous,ev_u8 state,ev_u8 flags,ev_u32 p1,ev_u32 p2);
ev_bool event_manager_get_status(ev_u8 module,ev_u8 *state);
ev_bool event_manager_thresholds(ev_u8 module,ev_u8 fail,ev_u8 good);
ev_u8 event_manager_global_status(void);
ev_u16 event_manager_count(void);
ev_bool event_manager_is_busy(void);
ev_bool event_manager_read(ev_u16 index);
void event_manager_time(ev_bool valid,ev_u32 utc);
ev_u16 event_reset_code(ev_u8 pcon);
#endif
