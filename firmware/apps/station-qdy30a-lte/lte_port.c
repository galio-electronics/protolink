/* Board adapter: UART5 and SIMCom control pins. */
#pin_select U5TX=TX_5
#pin_select U5RX=RX_5
#use rs232(baud=LTE_BAUD,parity=N,UART5,bits=8,stream=LTE_UART,errors)
static volatile unsigned int8 lte_rx[256];
static volatile unsigned int8 lte_head=0, lte_tail=0;
static volatile int1 lte_overflow=FALSE;
static char telemetry_buffer[240];
#INT_RDA5
void lte_rx_isr(void)
{
   unsigned int8 c, next;
   c=fgetc(LTE_UART);
   next=lte_head+1;
   if(next==lte_tail) lte_overflow=TRUE;
   else { lte_rx[lte_head]=c; lte_head=next; }
}
static char lte_port_read(void)
{
   char c=lte_rx[lte_tail];
   ++lte_tail;
   return c;
}
#define LTE_PUTC(c) fputc(c,LTE_UART)
#define LTE_AVAILABLE() (lte_head!=lte_tail)
#define LTE_GETC() lte_port_read()
#define LTE_MILLIS() protolink_millis()
#define LTE_RX_FAULT() lte_overflow
/* Only numeric MQTT results and CME errors; never echo commands/credentials. */
static void lte_trace_result(char *line)
{
   if((!strncmp(line,"+CMQTT",6) || !strncmp(line,"+CME ERROR:",11)) &&
      strlen(line)<48 && !strchr(line,'"'))
      printf(diagnostics_putc,"LTE RX %s\r\n",line);
}
#define LTE_AT_LINE_HOOK(line) lte_trace_result(line)
#include <lte-module/simcom/a7670.c>
static unsigned int8 lte_power_state=0;
static unsigned int32 lte_power_started;
static void lte_key(int1 active)
{
#if SIMCOM_KEY_ACTIVE_HIGH
   output_bit(SIMCOM_KEY,active);
   set_tris_d(get_tris_d() & 0xFB);
#else
   if(active) { output_low(SIMCOM_KEY); set_tris_d(get_tris_d() & 0xFB); }
   else set_tris_d(get_tris_d() | 4); // release PWRKEY to its own pull-up
#endif
}
static void lte_port_init(void)
{
   lte_key(FALSE);
   output_bit(SIMCOM_SLEEP,SIMCOM_SLEEP_AWAKE_HIGH);
   set_tris_d(get_tris_d() & 0xFD);
   lte_power_started=protolink_millis();
   output_high(TX_5);
   set_tris_e(0b11111011);
   setup_uart(LTE_BAUD,LTE_UART);
   enable_interrupts(INT_RDA5);
}

/* Probe before KEY: resetting the PIC must not switch off an awake modem. */
void lte_port_task(void)
{
   if(lte_power_state==4) { a7670_task(); return; }
   lte_at_poll();
   switch(lte_power_state)
   {
      case 0:
         if((unsigned int32)(protolink_millis()-lte_power_started)<SIMCOM_BOOT_MS) return;
         lte_at_command("AT","",3000); lte_power_state=1; break;
      case 1:
         if(lte_at_result==LTE_AT_WAIT) return;
         if(lte_at_result==LTE_AT_OK)
         { a7670_init(); lte_power_state=4; return; }
         lte_key(TRUE); lte_power_started=protolink_millis(); lte_power_state=2; break;
      case 2:
         if((unsigned int32)(protolink_millis()-lte_power_started)<SIMCOM_KEY_PULSE_MS) return;
         lte_key(FALSE); lte_power_started=protolink_millis(); lte_power_state=3; break;
      case 3:
         if((unsigned int32)(protolink_millis()-lte_power_started)<SIMCOM_BOOT_MS) return;
         a7670_init(); lte_power_state=4; break;
   }
}
