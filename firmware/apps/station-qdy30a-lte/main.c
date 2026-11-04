#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#pin_select U4TX=TX_4
#pin_select U4RX=RX_4
#byte qdy_tx4sta=getenv("SFR:TX4STA")
#bit qdy_tx_empty=qdy_tx4sta.1
#define SMODBUS_WAIT_TX_DONE() while(!qdy_tx_empty) { }
#include "../../common/library_config.h"
#include "lte_port.c"

static unsigned int32 last_poll=0, samples=0, failures=0;
static unsigned int16 registers[7];
static int1 rs485_led_on=FALSE;
static unsigned int32 rs485_led_started;

// Exact fixed-point formatting, including -32768 and negative fractions.
// Decimal count validated before calling. No float or locale-dependent JSON.
void qdy_format(signed int16 raw, unsigned int8 decimals, char *text)
{
   unsigned int32 magnitude, divisor=1, fraction;
   unsigned int8 i, pos=0;
   if(raw<0) { text[pos++]='-'; magnitude=-(signed int32)raw; }
   else magnitude=raw;
   for(i=0;i<decimals;i++) divisor*=10;
   sprintf(text+pos,"%LU",magnitude/divisor);
   pos=strlen(text);
   fraction=magnitude%divisor;
   if(decimals) text[pos++]='.';
   while(decimals--)
   {
      divisor/=10;
      text[pos++]='0'+(unsigned int8)(fraction/divisor);
      fraction%=divisor;
   }
   text[pos]=0;
}

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
   lte_port_init();
   printf(diagnostics_putc,"QDY UART4 addr=%u baud=%LU 8N1 LTE=UART5/115200\r\n",
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
   smodbus_status_t status;
   unsigned int8 error;
   char value[12], zero[12], full_scale[12];
   if((unsigned int32)(now-last_poll)<QDY_POLL_MS || !a7670_ready()) return;
   last_poll=now;
   // FC03 only. Never writes address, baudrate or calibration registers.
   status=smodbus_read_holding(QDY_ADDRESS,0,7,registers);
   error=status;
   if(status==SMODBUS_OK && (registers[0]!=QDY_ADDRESS || registers[3]>3)) error=5;
   rs485_led_on=(error==0);
   rs485_led_started=protolink_millis();
   output_bit(RS485_STATUS,rs485_led_on==RS485_STATUS_ACTIVE_HIGH);
   ++samples;
   if(error)
   {
      ++failures;
      sprintf(telemetry_buffer,"{\"sensor_ok\":false,\"modbus_error\":%u,\"sample\":%LU,\"errors\":%LU}\r\n",
         error,samples,failures);
      printf(diagnostics_putc,"QDY error=%u sample=%LU errors=%LU\r\n",error,samples,failures);
   }
   else
   {
      qdy_format((signed int16)registers[4],registers[3],value);
      qdy_format((signed int16)registers[5],registers[3],zero);
      qdy_format((signed int16)registers[6],registers[3],full_scale);
      // Unit code exposed explicitly: variants use different code tables.
      sprintf(telemetry_buffer,"{\"sensor_ok\":true,\"value\":%s,\"unit_code\":%LU,\"raw\":%LD,\"decimals\":%u,\"range_min\":%s,\"range_max\":%s,\"sample\":%LU}\r\n",
         value,(unsigned int32)registers[2],(signed int32)(signed int16)registers[4],
         (unsigned int8)registers[3],zero,full_scale,samples);
      printf(diagnostics_putc,"QDY value=%s unit_code=%LU raw=%LD decimals=%u baud_code=%LU\r\n",
         value,(unsigned int32)registers[2],(signed int32)(signed int16)registers[4],
         (unsigned int8)registers[3],(unsigned int32)registers[1]);
   }
   output_bit(LED2,(error==0)==((STATION_LED_ACTIVE_MASK & 2)!=0));
   a7670_publish(telemetry_buffer);
}
void app_task(void)
{
   unsigned int8 previous=a7670_state;
   lte_port_task();
   if(previous!=a7670_state)
   {
      printf(diagnostics_putc,"LTE step=%u -> %u result=%u\r\n",
         previous,a7670_state,lte_at_result);
      if(a7670_state==0 && !strncmp(lte_at_line,"+CME ERROR:",11))
         printf(diagnostics_putc,"LTE %s\r\n",lte_at_line);
   }
}
void app_task_1000ms(void)
{
   printf(diagnostics_putc,"LTE state=%u error=%u published=%LU\r\n",
      a7670_state,a7670_last_error,a7670_published);
}
#include "../../common/station_app.c"
