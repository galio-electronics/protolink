/* Adapters consume completed operations, never launch probes or recovery. */
#include "event_manager.h"
void event_health_lora(ev_bool response,ev_u32 driver_reason)
{ event_manager_report(EM_LORA,response ? EH_OK : EH_FAULT,0,driver_reason,0); }
void event_health_adc(ev_bool success,ev_u32 driver_reason,ev_u8 channel)
{ event_manager_report(EM_ADC,success ? EH_OK : EH_FAULT,0,driver_reason,channel); }
void event_health_rs485(ev_u8 connection,ev_u8 slave,ev_bool success,ev_u32 driver_reason)
{ event_manager_observe(EM_RS485,connection,success ? EH_OK : EH_FAULT,0,driver_reason,((ev_u32)connection<<8)|slave,0); }
void event_health_sd(ev_bool success,ev_u32 driver_reason,ev_u32 sector)
{ event_manager_observe(EM_SD,0,success ? EH_OK : EH_FAULT,0,driver_reason,sector,!success); }
