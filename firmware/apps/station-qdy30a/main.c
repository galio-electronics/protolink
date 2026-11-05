#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#include "../../common/event_station.c"
#include "../../common/event_health.c"
#pin_select U4TX=TX_4
#pin_select U4RX=RX_4
#byte qdy_tx4sta=getenv("SFR:TX4STA")
#bit qdy_tx_empty=qdy_tx4sta.1
#define SMODBUS_WAIT_TX_DONE() while(!qdy_tx_empty) { }
#include <simple_master/simple_master.c>
#define GALIO_SENSORS_CCS 1
#include <galio-sensors/adapters/modbus_simple_master.h>
#include <galio-sensors/sensors/qdy30a/qdy30a.c>
#include "ethernet_json.c"

static unsigned int32 last_poll=0, samples=0, failures=0;
static qdy30a_sample measurement;
static int1 rs485_led_on=FALSE;
static unsigned int32 rs485_led_started;

void app_init(void)
{
   output_high(TX_4);
   output_low(TX_ENABLE);
   output_low(RX_ENABLE); // DE low, /RE low: listening.
   output_bit(RS485_STATUS,!RS485_STATUS_ACTIVE_HIGH);
   set_tris_c(0b10111010); // C0 TX4, C1 RX4, C2 status LED, C6 debug TX.
   set_tris_h(0b11110000); // H0/H1 controls, H2/H3 DO remain inactive.
   setup_uart(SMODBUS_BAUD,SMODBUS_PORT);
   smodbus_init();
   ethernet_init();
   printf(diagnostics_putc,"QDY UART4 addr=%u baud=%LU 8N1 ETH=UART3/115200\r\n",
      QDY_ADDRESS,(unsigned int32)SMODBUS_BAUD);
}
void app_task_10ms(unsigned int8 inputs_raw)
{
   if(rs485_led_on && (unsigned int32)(protolink_millis()-rs485_led_started)>=RS485_STATUS_PULSE_MS)
   {
      output_bit(RS485_STATUS,!RS485_STATUS_ACTIVE_HIGH);
      rs485_led_on=FALSE;
   }
}
void app_task_100ms(void)
{
   unsigned int32 now=protolink_millis();
   unsigned int8 error;
   char value[12], zero[12], full_scale[12];
   if((unsigned int32)(now-last_poll)<QDY_POLL_MS || ethernet_sending) return;
   last_poll=now;
   // FC03 only. Never writes address, baudrate or calibration registers.
   error=qdy30a_read(QDY_ADDRESS,&measurement);
   event_health_rs485(0,QDY_ADDRESS,error==0,error);
   rs485_led_on=(error==0);
   rs485_led_started=protolink_millis();
   output_bit(RS485_STATUS,rs485_led_on==RS485_STATUS_ACTIVE_HIGH);
   ++samples;
   if(error)
   {
      ++failures;
      sprintf(ethernet_buffer,"{\"sensor_ok\":false,\"modbus_error\":%u,\"sample\":%LU,\"errors\":%LU}\r\n",
         error,samples,failures);
      printf(diagnostics_putc,"QDY error=%u sample=%LU errors=%LU\r\n",error,samples,failures);
   }
   else
   {
      qdy30a_format(measurement.raw,measurement.decimals,value);
      qdy30a_format(measurement.range_min,measurement.decimals,zero);
      qdy30a_format(measurement.range_max,measurement.decimals,full_scale);
      // Unit code exposed explicitly: variants use different code tables.
      sprintf(ethernet_buffer,"{\"sensor_ok\":true,\"value\":%s,\"unit_code\":%LU,\"raw\":%LD,\"decimals\":%u,\"range_min\":%s,\"range_max\":%s,\"sample\":%LU}\r\n",
         value,(unsigned int32)measurement.unit_code,(signed int32)measurement.raw,
         (unsigned int8)measurement.decimals,zero,full_scale,samples);
      printf(diagnostics_putc,"QDY value=%s unit_code=%LU raw=%LD decimals=%u baud_code=%LU\r\n",
         value,(unsigned int32)measurement.unit_code,(signed int32)measurement.raw,
         (unsigned int8)measurement.decimals,(unsigned int32)measurement.baud_code);
   }
   output_bit(LED2,(error==0)==((STATION_LED_ACTIVE_MASK & 2)!=0));
   ethernet_start();
}
void app_task_1000ms(void) { }
#include "../../common/station_app.c"
