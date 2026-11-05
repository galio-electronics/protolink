#ifndef EVENT_CONFIG_H
#define EVENT_CONFIG_H
/* All hardware enables are explicit. Monitor has no verified board port yet. */
#ifndef EVENT_STORAGE_ENABLED
#define EVENT_STORAGE_ENABLED 0
#endif
#ifndef EVENT_MODULE_MASK
#define EVENT_MODULE_MASK 0x0001
#endif
#ifndef EVENT_RS485_DEVICES
#define EVENT_RS485_DEVICES 1
#endif
#if EVENT_RS485_DEVICES < 1 || EVENT_RS485_DEVICES > 8
#error EVENT_RS485_DEVICES must be 1..8
#endif
#define EVENT_QUEUE_SIZE 4
#define EVENT_RECORD_SIZE 32
#define EVENT_SLOT_COUNT 1536
#define EVENT_BASE 0x4000
#define EVENT_FAIL_THRESHOLD 3
#define EVENT_OK_THRESHOLD 2
#define EVENT_WRITE_TIMEOUT_MS 10
#define EVENT_RETRY_MS 20
#define EVENT_RECOVERY_MS 5000
#define EVENT_RETRIES 3
#define EVENT_TIME_MAX_AGE_MS 2000
#ifdef EVENT_HOST
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t ev_u8;
typedef uint16_t ev_u16;
typedef uint32_t ev_u32;
typedef bool ev_bool;
#else
typedef unsigned int8 ev_u8;
typedef unsigned int16 ev_u16;
typedef unsigned int32 ev_u32;
typedef int1 ev_bool;
#endif
#endif
