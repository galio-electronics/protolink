#ifndef EVENT_STORAGE_H
#define EVENT_STORAGE_H
#include "event_crc.h"
#define STORAGE_INIT 0
#define STORAGE_SCANNING 1
#define STORAGE_READY 2
#define STORAGE_PREPARE 3
#define STORAGE_WRITE 4
#define STORAGE_WAIT_READY 5
#define STORAGE_VERIFY 6
#define STORAGE_ERROR 7
#define STORAGE_READING 8
#define STORAGE_DISABLED 9
/* Port calls: main only; bounded transaction, no write-cycle waiting. */
ev_bool event_port_read(ev_u16 address, ev_u8 *data);
ev_bool event_port_write(ev_u16 address, ev_u8 *data);
ev_bool event_port_ready(void);
ev_u32 event_port_millis(void);
extern ev_u8 event_storage_state;
extern ev_u16 event_storage_count, event_storage_boot;
extern ev_u32 event_storage_errors, event_storage_corrupt, event_scan_ms;
extern ev_bool event_storage_read_done, event_storage_read_ok;
extern ev_u8 event_storage_read_data[32];
void event_storage_init(void);
void event_storage_task(void);
ev_bool event_storage_submit(ev_u8 *record);
ev_bool event_storage_read(ev_u16 index);
ev_bool event_storage_clear(void);
#endif
