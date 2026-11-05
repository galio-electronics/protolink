#ifndef EVENT_CRC_H
#define EVENT_CRC_H
#include "event_config.h"
ev_u16 event_crc16(ev_u8 *data, ev_u8 length);
ev_u32 event_get_le(ev_u8 *data, ev_u8 length);
void event_put_le(ev_u8 *data, ev_u32 value, ev_u8 length);
ev_bool event_record_valid(ev_u8 *data);
ev_bool event_sequence_after(ev_u32 a, ev_u32 b);
#endif
