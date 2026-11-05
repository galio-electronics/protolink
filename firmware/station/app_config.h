#ifndef STATION_APP_CONFIG_H
#define STATION_APP_CONFIG_H
#define APP_NAME "station-master"
#define FW_VERSION "0.1.1"
#define PROTOLINK_DEFAULT 1
#define STATION_DEBUG_ENABLED 1
#define STATION_DEBUG_UART 1
#define STATION_DEBUG_BAUD 115200
#define APP_STATUS_INTERVAL_MS 1000
// UART1 pertenece a diagnostico. Activar otras librerias solo tras validar
// sus recursos y cableado: ver docs/dependencies.md y library_config.h.
#define STATION_PASS_STRINGS_IN_RAM 1
#define APP_EVENT_MANAGER 1
#define EVENT_STORAGE_ENABLED 1
#define EVENT_MODULE_MASK 0x0005
#endif
