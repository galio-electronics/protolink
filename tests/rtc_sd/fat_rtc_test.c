/* Host-only fault-injection harness. Never linked into PIC firmware. */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#define TRUE true
#define FALSE false
#define make8(v,n) ((uint8_t)((v)>>(8*(n))))
#define make16(h,l) (((uint16_t)(h)<<8)|(uint8_t)(l))
#define RTC_HAS_BATTERY 1
#define CLUSTERS 70000u
#define FATS 550u
#define DATA (32u+2*FATS)
#define SECTORS (DATA+CLUSTERS)
static uint8_t *disk;
static uint32_t base, ticks, reads, writes;
static uint32_t tick_step=1;
uint32_t sd_sector_count;
static bool ready;
static int fail_write=-1, fail_read=-1;
static uint32_t clock_ms(void) { return ticks; }
#define SD_MILLIS() clock_ms()
static bool sd_is_ready(void) { return ready; }
static bool sd_sync(void) { return ready; }
static bool sd_read_sector(uint32_t s, uint8_t *p) {
    ++reads; ticks+=tick_step;
    if(!ready || s>=sd_sector_count || fail_read==0) return false;
    if(fail_read>0) --fail_read;
    memcpy(p,disk+(size_t)s*512,512); return true;
}
static bool sd_write_sector(uint32_t s, uint8_t *p) {
    ++writes; ticks+=tick_step;
    if(!ready || s>=sd_sector_count || fail_write==0) return false;
    if(fail_write>0) --fail_write;
    memcpy(disk+(size_t)s*512,p,512); return true;
}
#include "fat32.c"
static void put16(uint8_t *p, uint16_t n) { p[0]=n; p[1]=n>>8; }
static void put32(uint8_t *p, uint32_t n) { put16(p,n); put16(p+2,n>>16); }
static uint32_t get32(uint8_t *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void setfat(uint32_t cluster, uint32_t value) {
    for(unsigned i=0;i<2;i++) put32(disk+(size_t)(base+32+i*FATS)*512+cluster*4,value);
}
static void volume(bool mbr) {
    base=mbr?2048:0; sd_sector_count=SECTORS+base;
    free(disk); disk=calloc(sd_sector_count,512); assert(disk);
    uint8_t *p=disk+(size_t)base*512;
    if(mbr) { disk[450]=0x0C; put32(disk+454,base); put32(disk+458,SECTORS); put16(disk+510,0xAA55); }
    put16(p+11,512); p[13]=1; put16(p+14,32); p[16]=2;
    put32(p+32,SECTORS); put32(p+36,FATS); put32(p+44,2);
    put16(p+48,1); put16(p+50,6); put16(p+510,0xAA55);
    for(unsigned i=0;i<2;i++) {
        p=disk+(size_t)(base+1+i*6)*512;
        put32(p,0x41615252); put32(p+484,0x61417272); put32(p+508,0xAA550000);
        put32(p+488,CLUSTERS-1); put32(p+492,3);
    }
    setfat(0,0x0FFFFFF8); setfat(1,0x0FFFFFFF); setfat(2,0x0FFFFFFF);
    ready=true; fail_read=fail_write=-1; ticks=reads=writes=0;tick_step=1;
    assert(fat32_mount()); assert(writes==0);
}
static void file_tests(bool mbr) {
    char input[1300], output[1300];
    for(unsigned i=0;i<sizeof(input);i++) input[i]=(char)(i%97+32);
    volume(mbr);
    assert(fat32_open("T261103 CSV",true));
    assert(fat32_append(input,510,0x5D63,0x9000));
    assert(fat32_append(input+510,790,0x5D63,0x9000));
    assert(fat32_close());
    assert(fat32_mount());
    assert(fat32_open("T261103 CSV",false)); assert(fat32_file_size==1300);
    assert(fat32_read_at(0,output,1300)); assert(!memcmp(input,output,1300));
    assert(fat32_close());
    assert(!memcmp(disk+(size_t)(base+32)*512,disk+(size_t)(base+32+FATS)*512,FATS*512));
    assert(get32(disk+(size_t)(base+1)*512+488)==UINT32_MAX);
    assert(get32(disk+(size_t)(base+7)*512+488)==UINT32_MAX);
    assert(fat32_open("E261104 CSV",true)); assert(fat32_append("next day\r\n",10,0x5D64,0));
    assert(fat32_open("T261103 CSV",false)); assert(fat32_file_size==1300);
    /* A self-cycle must be rejected before the first write. */
    setfat(fat_file_first,fat_file_first); unsigned before=writes;
    assert(!fat32_open("T261103 CSV",true)); assert(fat32_error==FAT_CORRUPT);
    assert(writes==before);
}
static void fault_tests(void) {
    volume(false);
    assert(fat32_open("FULL    CSV",true));
    for(uint32_t c=3;c<CLUSTERS+2;c++) setfat(c,0x0FFFFFFF);
    assert(!fat32_append("x",1,0,0)); assert(fat32_error==FAT_FULL);
    volume(false); assert(fat32_open("FAULT   CSV",true)); fail_write=0;
    assert(!fat32_append("x",1,0,0)); assert(!fat32_is_ready());
    volume(false); fail_read=0; assert(!fat32_mount()); assert(fat32_error==FAT_IO);
    volume(false); put16(disk+11,4096); assert(!fat32_mount());
    volume(false); put32(disk+32,UINT32_MAX); assert(!fat32_mount());
    volume(false); put32(disk+36,1); assert(!fat32_mount());
    volume(false); put16(disk+40,0x80); assert(!fat32_mount());
    volume(false); ready=false; assert(!fat32_mount());
    volume(false);
    uint8_t *root=disk+(size_t)DATA*512;
    memset(root,'A',512); for(unsigned i=0;i<16;i++) root[i*32+11]=0x20;
    assert(!fat32_open("NEW     CSV",true)); assert(fat32_error==FAT_DIR_FULL);
    volume(false); assert(fat32_open("SLOW    CSV",true));
    for(uint32_t c=3;c<1024;c++) setfat(c,0x0FFFFFFF);
    tick_step=1000; assert(!fat32_append("x",1,0,0)); assert(fat32_error==FAT_LIMIT);
    /* End marker at sector boundary must hide stale next-sector entries. */
    volume(false); setfat(2,3); setfat(3,0x0FFFFFFF);
    root=disk+(size_t)DATA*512; memset(root,'A',480); root[480]=0;
    for(unsigned i=0;i<15;i++) root[i*32+11]=0x20;
    memset(root+512,'Z',512);
    assert(fat32_open("LAST    CSV",true)); assert(root[512]==0);
    puts("PASS FAT32: MBR/superfloppy, append/reopen/read, mirrors, FSInfo, day files, full, corrupt, IO, geometry, directory");
}
static uint8_t rtc_regs[0x20];
static bool rtc_bus_ok=true;
static void rtc_bus_init(void) {}
static void delay_ms(unsigned n) { ticks+=n; }
static bool rtc_bus_read(uint8_t reg,uint8_t *data,uint8_t n) {
    if(!rtc_bus_ok) return false; memcpy(data,rtc_regs+reg,n); return true;
}
static bool rtc_bus_write(uint8_t reg,uint8_t *data,uint8_t n) {
    if(!rtc_bus_ok) return false;
    if(reg<=3 && reg+n>3 && !(data[3-reg]&0x10)) memset(rtc_regs+0x18,0,8);
    memcpy(rtc_regs+reg,data,n);
    if(rtc_regs[0]&0x80) rtc_regs[3]|=0x20; else rtc_regs[3]&=~0x20;
    return true;
}
#include "mcp79402.c"
static void rtc_tests(void) {
    rtc_datetime_t dt={2026,11,3,19,30,0}, got;
    assert(rtc_init()); assert(!rtc_is_valid()); assert(rtc_error==RTC_STOPPED);
    assert(rtc_set_datetime(&dt)); assert(rtc_get_datetime(&got));
    assert(got.year==2026 && got.month==11 && got.day==3 && got.hour==19);
    assert((rtc_regs[3]&7)==3); /* Tuesday, Sunday=1 */
    uint8_t copy[7]; memcpy(copy,rtc_regs,7); assert(rtc_init()); assert(!memcmp(copy,rtc_regs,7));
    dt.year=2024;dt.month=2;dt.day=29;assert(rtc_validate(&dt));
    dt.year=2025; assert(!rtc_set_datetime(&dt));
    dt.year=2100; assert(!rtc_validate(&dt));
    rtc_regs[1]=0x6A;assert(!rtc_is_valid());rtc_regs[1]=0x30;
    rtc_regs[3]|=0x10; memset(rtc_regs+0x18,0x11,8);
    assert(rtc_init());assert(rtc_get_power_fail_status());assert(!rtc_is_valid());
    assert(rtc_error==RTC_POWER_FAIL);assert(rtc_regs[0x18]==0x11);
    assert(rtc_clear_power_fail_status());assert(rtc_regs[0x18]==0);assert(rtc_is_valid());
    assert(rtc_enable_sqw());assert(rtc_regs[7]==0x40);
    rtc_bus_ok=false;assert(!rtc_is_valid());assert(rtc_error==RTC_BUS_ERROR);
    puts("PASS RTC: calendar/leap/BCD, weekday, preserve boot time, stop, powerfail/clear, SQW, missing device");
}
static void callback_printf(void (*out)(char), const char *format, ...) {
    char text[512]; va_list args;va_start(args,format);
    int n=vsnprintf(text,sizeof(text),format,args);va_end(args);
    assert(n>=0 && n<(int)sizeof(text));for(int i=0;i<n;i++)out(text[i]);
}
#define LOGGER_RECORD_MAX 128
#define LOGGER_QUEUE_SIZE 2
#define LOGGER_ENABLE 1
#define LOGGER_EVENTS_ENABLE 1
#define LOGGER_TELEMETRY_ENABLE 1
#define LOGGER_LEVEL 1
#define LOGGER_RETRY_MS 10000u
static uint32_t protolink_millis(void) { return ticks; }
static bool sd_init(void) { return ready; }
#include "station_logger.c"
static void logger_tests(void) {
    rtc_datetime_t dt={2026,11,3,23,59,59};
    volume(true); rtc_bus_ok=true;assert(rtc_set_datetime(&dt));
    logger_init();assert(!logger_enabled);logger_enabled=true;
    assert(logger_telemetry("STATION01,DI1,1,bool,GOOD"));
    dt.day=4;dt.hour=0;dt.minute=0;dt.second=0;assert(rtc_set_datetime(&dt));
    assert(logger_telemetry("STATION01,DI1,0,bool,GOOD"));
    assert(!logger_event(LOG_INFO,"TEST","queue full"));assert(logger_lost==1);
    logger_flush();assert(logger_count==0);assert(logger_written==2);
    char data[128];
    assert(fat32_open("T261103 CSV",false));assert(fat32_file_size<sizeof(data));
    assert(fat32_read_at(0,data,fat32_file_size));data[fat32_file_size]=0;
    assert(strstr(data,"timestamp,device_id,signal,value,unit,quality\r\n"));
    assert(strstr(data,"2026-11-03T23:59:59,STATION01,DI1,1,bool,GOOD\r\n"));
    assert(fat32_open("T261104 CSV",false));assert(fat32_file_size>0);
    assert(logger_event_code(LOG_INFO,"SYSTEM","BOOT","System started"));
    logger_flush();assert(fat32_open("E261104 CSV",false));assert(fat32_file_size>0);
    assert(!logger_event(LOG_INFO,"TEST","bad,comma"));
    assert(!logger_telemetry("not CSV"));
    char big[128];memset(big,'A',127);big[127]=0;
    assert(!logger_event(LOG_INFO,"TEST",big));assert(logger_count==0);
    assert(logger_telemetry("STATION01,DI1,1,bool,GOOD"));
    fail_write=0;logger_task();assert(!logger_enabled);assert(logger_count==0);
    puts("PASS LOGGER: bounded queue/overflow, CSV, midnight rotation, headers, invalid input, failed-write stop");
}
int main(void) { file_tests(false); file_tests(true); fault_tests(); rtc_tests(); logger_tests(); free(disk); return 0; }
