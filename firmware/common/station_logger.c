#include "station_logger.h"
typedef struct {
   char text[LOGGER_RECORD_MAX];
   unsigned int16 date, time;
   unsigned int8 length, year, month, day;
   char kind;
} logger_record_t;
static logger_record_t logger_queue[LOGGER_QUEUE_SIZE];
static unsigned int8 logger_head=0, logger_tail=0, logger_count=0;
static unsigned int32 logger_retry_at=0;
unsigned int32 logger_lost=0, logger_written=0, logger_max_write_ms=0;
int1 logger_enabled=FALSE;
static char logger_name[12];
static int1 logger_bad=FALSE;
static unsigned int8 logger_length=0;
/* Bounded printf callback; an oversized record is dropped as a whole. */
static void logger_putc(char c)
{
   if(logger_length>=LOGGER_RECORD_MAX-1) logger_bad=TRUE;
   else logger_queue[logger_head].text[logger_length++]=c;
}
static int1 logger_field(char *text, int1 commas)
{
   unsigned int16 i;
   for(i=0;i<LOGGER_RECORD_MAX;i++) {
      if(!text[i]) return i!=0;
      if(text[i]<32 || text[i]>126 || text[i]=='"' || (!commas && text[i]==',')) return FALSE;
   }
   return FALSE;
}
static int1 logger_begin(char kind)
{
   rtc_datetime_t dt;
   if(!LOGGER_ENABLE || !logger_enabled) return FALSE;
   if(logger_count==LOGGER_QUEUE_SIZE || !rtc_get_datetime(&dt)) { ++logger_lost; return FALSE; }
   logger_length=0; logger_bad=FALSE;
   logger_queue[logger_head].kind=kind;
   logger_queue[logger_head].year=dt.year-2000;
   logger_queue[logger_head].month=dt.month; logger_queue[logger_head].day=dt.day;
   logger_queue[logger_head].date=((dt.year-1980)<<9)|((unsigned int16)dt.month<<5)|dt.day;
   logger_queue[logger_head].time=((unsigned int16)dt.hour<<11)|((unsigned int16)dt.minute<<5)|(dt.second/2);
   printf(logger_putc,"%04Lu-%02u-%02uT%02u:%02u:%02u,",dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);
   return TRUE;
}
static int1 logger_end(void)
{
   logger_putc('\r'); logger_putc('\n');
   if(logger_bad) { ++logger_lost; return FALSE; }
   logger_queue[logger_head].text[logger_length]=0;
   logger_queue[logger_head].length=logger_length;
   logger_head=(logger_head+1)%LOGGER_QUEUE_SIZE; ++logger_count;
   return TRUE;
}
int1 logger_event_code(unsigned int8 level, char *source, char *event, char *message)
{
   if(!LOGGER_EVENTS_ENABLE || level<LOGGER_LEVEL || level>LOG_CRITICAL) return FALSE;
   if(!logger_field(source,FALSE) || !logger_field(event,FALSE) || !logger_field(message,FALSE)) {
      ++logger_lost; return FALSE;
   }
   if(!logger_begin('E')) return FALSE;
   switch(level) {
      case LOG_DEBUG: printf(logger_putc,"DEBUG,"); break;
      case LOG_INFO: printf(logger_putc,"INFO,"); break;
      case LOG_WARNING: printf(logger_putc,"WARNING,"); break;
      case LOG_ERROR: printf(logger_putc,"ERROR,"); break;
      default: printf(logger_putc,"CRITICAL,"); break;
   }
   printf(logger_putc,"%s,%s,%s",source,event,message);
   return logger_end();
}
int1 logger_event(unsigned int8 level, char *source, char *message)
{ return logger_event_code(level,source,"MESSAGE",message); }
int1 logger_telemetry(char *data)
{
   unsigned int8 i, commas=0;
#if (LOGGER_TELEMETRY_ENABLE == 0)
   return FALSE;
#endif
   if(!logger_field(data,TRUE)) { ++logger_lost; return FALSE; }
   for(i=0;data[i];i++) if(data[i]==',') ++commas;
   if(commas!=4) { ++logger_lost; return FALSE; }
   if(!logger_begin('T')) return FALSE;
   printf(logger_putc,"%s",data);
   return logger_end();
}
void logger_init(void)
{
   logger_head=logger_tail=logger_count=0;
   logger_enabled=FALSE; logger_lost=logger_written=logger_max_write_ms=0;
   logger_retry_at=protolink_millis();
}
static int1 logger_write_record(void)
{
   char header[64];
   unsigned int16 n;
   logger_record_t *record=&logger_queue[logger_tail];
   sprintf(logger_name,"%c%02u%02u%02u CSV",record->kind,record->year,record->month,record->day);
   if(!fat32_open(logger_name,TRUE)) return FALSE;
   if(fat32_file_size==0) {
      if(record->kind=='T') strcpy(header,"timestamp,device_id,signal,value,unit,quality\r\n");
      else strcpy(header,"timestamp,level,source,event,message\r\n");
      n=strlen(header);
      if(!fat32_append(header,n,record->date,record->time)) return FALSE;
   }
   if(!fat32_append(record->text,record->length,record->date,record->time)) return FALSE;
   return fat32_close();
}
void logger_task(void)
{
   unsigned int32 start, elapsed;
#if (LOGGER_ENABLE == 0)
   return;
#endif
   if(!fat32_is_ready()) {
      if((unsigned int32)(protolink_millis()-logger_retry_at)<LOGGER_RETRY_MS) return;
      logger_retry_at=protolink_millis();
      if(!sd_init() || !fat32_mount()) return;
   }
   if(!logger_count) return;
   start=protolink_millis();
   if(logger_write_record()) ++logger_written;
   else {
      ++logger_lost;
      /* A failed append is ambiguous: do not replay it automatically. Require
       * operator acknowledgement before more writes; reads/reinit may retry. */
      logger_enabled=FALSE;
      logger_lost+=logger_count-1;
      logger_count=1; logger_head=(logger_tail+1)%LOGGER_QUEUE_SIZE;
   }
   elapsed=protolink_millis()-start;
   if(elapsed>logger_max_write_ms) logger_max_write_ms=elapsed;
   logger_tail=(logger_tail+1)%LOGGER_QUEUE_SIZE; --logger_count;
}
void logger_flush(void)
{
   unsigned int8 remaining=logger_count;
   while(remaining-- && logger_count && fat32_is_ready()) logger_task();
}
