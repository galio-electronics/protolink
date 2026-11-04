#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#include "rtc_port.c"
#include "../../common/mcp79402.c"
#include "sd_port.c"
#include <sd-card/sd_card.c>
#include <sd-card/fat32.c>
#include "../../common/station_logger.c"

static unsigned int32 demo_last_log=0, demo_mfp_edges=0;
static int1 demo_mfp_last=FALSE;
static unsigned int8 demo_inputs=0;
static void demo_rtc(void)
{
   rtc_datetime_t dt;
   if(rtc_get_datetime(&dt))
      printf(diagnostics_putc,"RTC %04Lu-%02u-%02uT%02u:%02u:%02u mfp_edges=%LU\r\n",
             dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second,demo_mfp_edges);
   else printf(diagnostics_putc,"RTC error=%u mfp_edges=%LU\r\n",rtc_error,demo_mfp_edges);
}
static void demo_storage(void)
{
   printf(diagnostics_putc,"SD ready=%u err=%u sectors=%LU FAT ready=%u err=%u\r\n",
          sd_is_ready(),sd_error,sd_sector_count,fat32_is_ready(),fat32_error);
}
static void demo_sample(void)
{
   char payload[64];
   if(strlen(LOGGER_DEVICE_ID)>16 || strlen(LOGGER_SIGNAL_NAME)>16) {
      ++logger_lost; return;
   }
   sprintf(payload,"%s,%s,%u,bitmask,GOOD",LOGGER_DEVICE_ID,LOGGER_SIGNAL_NAME,demo_inputs);
   logger_telemetry(payload);
}
static int1 demo_set_clock(char *command)
{
   rtc_datetime_t dt;
   unsigned int8 i, v[7];
   if(strlen(command)!=18) return FALSE;
   for(i=4;i<18;i++) if(command[i]<'0' || command[i]>'9') return FALSE;
   for(i=0;i<7;i++) v[i]=(command[4+2*i]-'0')*10+command[5+2*i]-'0';
   dt.year=(unsigned int16)v[0]*100+v[1]; dt.month=v[2]; dt.day=v[3];
   dt.hour=v[4]; dt.minute=v[5]; dt.second=v[6];
   return rtc_set_datetime(&dt);
}
int1 demo_command(char *command)
{
   unsigned int8 raw[8], i;
   unsigned int16 n;
   char check[65];
   if(!strcmp(command,"rtc")) demo_rtc();
   else if(!strcmp(command,"rtc raw")) {
      if(rtc_read(0,raw,8)) {
         printf(diagnostics_putc,"RTC regs 00..07:");
         for(i=0;i<8;i++) printf(diagnostics_putc," %02X",raw[i]);
         printf(diagnostics_putc,"\r\n");
      }
   }
   else if(!strncmp(command,"rtc ",4)) {
      printf(diagnostics_putc,"RTC set=%u (rtc YYYYMMDDhhmmss)\r\n",demo_set_clock(command));
   }
   else if(!strcmp(command,"power")) {
      if(rtc_get_power_timestamps(raw)) {
         printf(diagnostics_putc,"POWER raw BCD down/up min hour day month:");
         for(i=0;i<8;i++) printf(diagnostics_putc," %02X",raw[i]);
         printf(diagnostics_putc," flag=%u\r\n",rtc_get_power_fail_status());
      } else printf(diagnostics_putc,"RTC error=%u\r\n",rtc_error);
   }
   else if(!strcmp(command,"power clear"))
      printf(diagnostics_putc,"POWER clear=%u timestamps erased\r\n",rtc_clear_power_fail_status());
   else if(!strcmp(command,"sd")) demo_storage();
   else if(!strcmp(command,"sd init")) {
      logger_flush();
      if(sd_init()) fat32_mount();
      demo_storage();
   }
   else if(!strcmp(command,"sd sector0")) {
      if(sd_read_sector(0,fat32_buffer))
         printf(diagnostics_putc,"LBA0 signature=%02X%02X\r\n",fat32_buffer[511],fat32_buffer[510]);
      else demo_storage();
   }
   else if(!strcmp(command,"log on")) {
      logger_enabled=fat32_is_ready() && rtc_is_valid();
      if(logger_enabled) logger_event_code(LOG_INFO,"SYSTEM","LOG_START","Demo logging enabled");
      printf(diagnostics_putc,"LOG enabled=%u\r\n",logger_enabled);
   }
   else if(!strcmp(command,"log off")) {
      logger_enabled=FALSE; logger_flush();
      printf(diagnostics_putc,"LOG off pending=%u\r\n",logger_count);
   }
   else if(!strcmp(command,"log now")) { demo_sample(); }
   else if(!strcmp(command,"log status"))
      printf(diagnostics_putc,"LOG on=%u queued=%u written=%LU lost=%LU max_ms=%LU\r\n",
             logger_enabled,logger_count,logger_written,logger_lost,logger_max_write_ms);
   else if(!strcmp(command,"log read")) {
      logger_flush();
      if(logger_name[0] && fat32_open(logger_name,FALSE)) {
         n=fat32_file_size>64 ? 64 : fat32_file_size;
         if(fat32_read_at(fat32_file_size-n,check,n)) {
            check[n]=0; printf(diagnostics_putc,"TAIL %s",check);
         }
         fat32_close();
      } else printf(diagnostics_putc,"READ no last file err=%u\r\n",fat32_error);
   }
   else if(!strcmp(command,"help"))
      printf(diagnostics_putc,"rtc [YYYYMMDDhhmmss], power [clear], sd [init|sector0], log on|off|now|status|read\r\n");
   else return FALSE;
   return TRUE;
}
void app_init(void)
{
   logger_init();
   if(rtc_init()) rtc_enable_sqw();
   if(sd_init()) fat32_mount();
   demo_mfp_last=input(RTC_MFP);
   demo_last_log=protolink_millis();
   printf(diagnostics_putc,"DEMO RTC+SD. Read-only until 'log on'. Type help.\r\n");
}
void app_task(void) { logger_task(); }
void app_task_10ms(unsigned int8 inputs_raw)
{
   int1 level=input(RTC_MFP);
   demo_inputs=inputs_raw;
   if(level && !demo_mfp_last) ++demo_mfp_edges;
   demo_mfp_last=level;
}
void app_task_100ms(void) { }
void app_task_1000ms(void)
{
   demo_rtc();
   if((unsigned int32)(protolink_millis()-demo_last_log)>=LOGGER_INTERVAL_SEC*1000UL) {
      demo_last_log=protolink_millis();
      demo_sample();
   }
}
#include "../../common/station_app.c"
