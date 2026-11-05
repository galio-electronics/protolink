#include <stdio.h>
#include <string.h>
#include <assert.h>
#define EVENT_HOST 1
#define EVENT_STORAGE_ENABLED 1
#define EVENT_MODULE_MASK 255
#define EVENT_RS485_DEVICES 2
#include "event_manager.h"
static ev_u8 memory[65536];
static ev_u32 now_ms,writes,ready_at;
static ev_bool fail_bus,protect;
static int tear=-1;
ev_u32 event_port_millis(void) { return now_ms; }
ev_bool event_port_ready(void) { return !fail_bus && now_ms>=ready_at; }
ev_bool event_port_read(ev_u16 addr,ev_u8 *data)
{
   assert(addr>=0x4000 && !(addr&31));
   if(fail_bus) return 0;
   memcpy(data,memory+addr,32); return 1;
}
ev_bool event_port_write(ev_u16 addr,ev_u8 *data)
{
   assert(addr>=0x4000 && !(addr&31) && (addr&127)<=96);
   if(fail_bus) return 0;
   ++writes;
   if(!protect) memcpy(memory+addr,data,tear<0 ? 32 : tear);
   ready_at=now_ms+5; return 1;
}
#include "event_crc.c"
#include "event_storage.c"
#include "event_manager.c"
#include "event_health.c"
static void ticks(unsigned n) { while(n--) { ++now_ms; event_manager_task(); } }
static void drain(void)
{
   unsigned budget=30000;
   while(event_manager_is_busy() && budget--) ticks(1);
   assert(!event_manager_is_busy());
}
static void fresh(void)
{
   memset(memory,255,sizeof memory); memset(memory,0xA5,0x4000);
   now_ms=0; writes=0; fail_bus=0; protect=0; tear=-1;
   event_manager_init(); drain(); assert(writes==0);
}
static void emit(ev_u32 param)
{ assert(event_manager_emit(EM_SYSTEM,EV_SYSTEM_BOOT,EH_UNKNOWN,EH_OK,ES_INFO,param,0)); }
static ev_u32 read_param(ev_u16 index)
{
   assert(event_manager_read(index));
   ticks(1600);
   assert(event_storage_read_done && event_storage_read_ok);
   return event_get_le(event_storage_read_data+18,4);
}
static void install(ev_u16 slot,ev_u32 seq)
{
   ev_u8 *r=memory+0x4000+slot*32;
   memset(r,0,32); event_put_le(r,seq,4); event_put_le(r+12,EV_SYSTEM_BOOT,2);
   event_put_le(r+30,event_crc16(r,30),2);
}
int main(void)
{
   static const ev_u8 golden[32]={
      0x00,0x00,0x00,0x00,0x00,0xF1,0x53,0x65,0x7B,0x00,0x00,0x00,
      0x01,0x01,0x00,0x00,0x01,0x00,0x78,0x56,0x34,0x12,0xF0,0xDE,
      0xBC,0x9A,0x01,0x00,0x00,0x00,0xCB,0xF9};
   ev_u8 bytes[32],state;
   ev_u32 i,oldwrites;
   assert(event_crc16((ev_u8 *)"123456789",9)==0x29B1);
   memset(bytes,0,32); event_put_le(bytes,0x12345678UL,4);
   assert(bytes[0]==0x78 && bytes[3]==0x12 && event_get_le(bytes,4)==0x12345678UL);
   event_put_le(bytes+30,event_crc16(bytes,30),2); assert(event_record_valid(bytes));
   bytes[17]^=1; assert(!event_record_valid(bytes));
   fresh(); now_ms=123000; event_manager_time(1,1700000000UL);
   assert(event_manager_emit(EM_SYSTEM,EV_SYSTEM_BOOT,EH_UNKNOWN,EH_OK,ES_INFO,0x12345678UL,0x9ABCDEF0UL));
   drain(); assert(read_param(0)==0x12345678UL);
   assert(!memcmp(event_storage_read_data,golden,32));
   fresh(); emit(42); drain(); assert(event_manager_count()==1 && read_param(0)==42);
   assert(event_storage_read_data[17]&EF_TIME_INVALID);
   assert(event_get_le(event_storage_read_data+26,2)==1);
   event_manager_init(); drain(); assert(event_manager_count()==1); emit(43); drain();
   assert(read_param(1)==43 && event_get_le(event_storage_read_data+26,2)==2);
   fresh();
   for(i=0;i<1600;i++) { emit(i); drain(); }
   assert(event_manager_count()==1536 && read_param(0)==64 && read_param(1535)==1599);
   event_manager_init(); drain(); assert(read_param(0)==64 && read_param(1535)==1599);
   for(i=0;i<0x4000;i++) assert(memory[i]==0xA5);
   fresh(); install(0,0xFFFFFFFEUL); install(1,0xFFFFFFFFUL); install(2,0);
   event_manager_init(); drain(); emit(7); drain();
   assert(read_param(3)==7 && event_get_le(event_storage_read_data,4)==1);
   assert(event_sequence_after(0,0xFFFFFFFFUL) && !event_sequence_after(0xFFFFFFFFUL,0));
   fresh(); emit(1); drain(); memory[0x4000+5]^=0x80;
   event_manager_init(); drain(); assert(event_storage_corrupt==1); /* CRC diagnostic appended */
   fresh(); emit(1); drain(); tear=12; emit(2); ticks(100); assert(event_storage_state==STORAGE_ERROR);
   tear=-1; event_manager_init(); drain(); assert(read_param(0)==1 && event_storage_corrupt==1);
   /* Every interrupted prefix, including both CRC bytes, must preserve prior history. */
   for(i=0;i<32;i++) {
      fresh(); emit(100); drain(); tear=i; emit(200); ticks(3);
      tear=-1; event_manager_init(); drain(); assert(read_param(0)==100);
      assert(event_storage_count<=2); /* optional CRC diagnostic, never torn event */
   }
   fresh(); event_manager_init(); emit(10); emit(11);
   ticks(10); assert(event_storage_state==STORAGE_SCANNING && event_queue_count==2);
   drain(); assert(read_param(0)==10 && read_param(1)==11);
   fresh();
   for(i=0;i<4;i++) emit(i);
   assert(!event_manager_emit(EM_SYSTEM,EV_SYSTEM_BOOT,0,1,ES_INFO,9,0));
   assert(event_manager_emit(EM_SYSTEM,EV_RESET_WATCHDOG,0,1,ES_CRITICAL,10,0));
   assert(event_queue_lost==2); drain(); assert(read_param(0)==1 && read_param(3)==10);
   fresh();
   for(i=0;i<2;i++) event_health_lora(1,0);
   assert(event_queue_count==0);
   for(i=0;i<2;i++) event_health_lora(0,1);
   event_manager_get_status(EM_LORA,&state); assert(state==EH_OK);
   event_health_lora(0,1); assert(event_queue_count==1);
   event_health_lora(0,1); assert(event_queue_count==1);
   event_health_lora(1,0); event_health_lora(1,0);
   assert(event_queue_count==2); drain();
   for(i=0;i<3;i++) event_health_adc(0,1,0);
   event_manager_get_status(EM_ADC,&state); assert(state==EH_FAULT);
   for(i=0;i<2;i++) event_health_adc(1,0,0);
   event_manager_get_status(EM_ADC,&state); assert(state==EH_OK); drain();
   event_health_sd(0,1,12); event_manager_get_status(EM_SD,&state); assert(state==EH_FAULT);
   event_health_sd(1,0,12); event_health_sd(1,0,12); drain();
   for(i=0;i<3;i++) event_health_rs485(1,17,0,1);
   event_manager_get_status(EM_RS485,&state); assert(state==EH_FAULT);
   assert(!event_manager_observe(EM_RS485,2,EH_OK,0,0,0,0)); drain();
   fresh(); fail_bus=1; emit(1); ticks(100);
   assert(event_storage_state==STORAGE_ERROR && event_queue_count==1);
   oldwrites=writes; ticks(100); assert(writes==oldwrites);
   fail_bus=0; drain(); assert(event_manager_count()>=2);
   fresh(); protect=1; emit(3); ticks(100); assert(event_storage_state==STORAGE_ERROR);
   protect=0; drain(); assert(read_param(0)==3);
   fresh(); event_manager_time(1,1700000000UL); emit(1); drain(); read_param(0);
   assert(event_get_le(event_storage_read_data+4,4)==1700000000UL);
   ticks(2100); emit(2); drain(); read_param(1); assert(event_storage_read_data[17]&EF_TIME_INVALID);
   assert(event_reset_code(0x3C)==0 && event_reset_code(0x3E)==EV_RESET_BROWNOUT);
   assert(event_reset_code(0x2F)==EV_RESET_WATCHDOG && event_reset_code(0x37)==EV_RESET_EXTERNAL);
   assert(event_reset_code(0x3B)==EV_RESET_SOFTWARE && event_reset_code(0xBF)==EV_RESET_STACK_FAULT);
   assert(event_reset_code(0x3F)==EV_RESET_UNKNOWN && event_reset_code(0)==EV_RESET_UNKNOWN);
   fresh(); emit(1); drain(); assert(event_storage_clear()); drain(); assert(event_manager_count()==0);
   event_manager_init(); drain(); assert(event_manager_count()==0);
   puts("PASS event core: CRC, encoding, scan, reboot, ring, sequence wrap, tears, queue, health, bus errors, WP, time, resets, clear, protected region");
   return 0;
}
