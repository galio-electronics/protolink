/* Scripted SPI card, host tests only. Exercises production SD C state flow. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define TRUE true
#define FALSE false
#define make8(v,n) ((uint8_t)((v)>>(8*(n))))
static bool sd_port_fault, selected, hc=true, absent, bad_echo, init_busy, bad_token, reject_write;
static uint32_t ticks, last_address;
static unsigned fast_calls, clocks_high, commands[64];
static uint8_t packet[6], response[520], storage[512];
static unsigned packet_n, response_n, response_at, write_n;
static bool writing;
#define SD_MILLIS() (ticks)
static void delay_ms(unsigned n) { ticks+=n; }
static void sd_port_select(bool active) {
    selected=active;
    if(!active) { packet_n=0; response_n=response_at=0; writing=false; }
}
static void sd_port_init(void) { sd_port_fault=false; selected=false; }
static void sd_port_fast(void) { ++fast_calls; }
static void respond(uint8_t v) { response[response_n++]=v; }
static void process_command(void) {
    unsigned cmd=packet[0]&63;
    uint32_t arg=(uint32_t)packet[1]<<24|(uint32_t)packet[2]<<16|(uint32_t)packet[3]<<8|packet[4];
    ++commands[cmd]; response_n=response_at=0;
    switch(cmd) {
        case 0: assert(packet[5]==0x95);respond(1);break;
        case 8:
            assert(packet[5]==0x87);assert(arg==0x1AA);
            respond(hc?1:5);
            if(hc) {respond(0);respond(0);respond(1);respond(bad_echo?0:0xAA);}break;
        case 55: respond(1);break;
        case 41: assert(arg==(hc?0x40000000:0));respond(init_busy?1:0);break;
        case 58: respond(0);respond(hc?0xC0:0x80);respond(0x30);respond(0);respond(0);break;
        case 16: assert(!hc && arg==512);respond(0);break;
        case 9: {
            uint8_t csd[16]={0};
            if(hc) {csd[0]=0x40;csd[9]=127;} /* 131072 sectors */
            else {csd[5]=9;csd[6]=0;csd[7]=255;csd[8]=0xC0;csd[9]=3;csd[10]=0x80;}
            respond(0);respond(0xFE);for(unsigned i=0;i<16;i++)respond(csd[i]);respond(0);respond(0);break;
        }
        case 17:
            last_address=arg;respond(0);respond(bad_token?0x0B:0xFE);
            for(unsigned i=0;i<512;i++)respond(storage[i]);respond(0);respond(0);break;
        case 24: last_address=arg;respond(0);writing=true;write_n=0;break;
        case 13: respond(0);respond(0);break;
        default: assert(!"unexpected command");
    }
}
static uint8_t sd_port_xfer(uint8_t value) {
    ++ticks;
    if(!selected) {++clocks_high;return 0xFF;}
    if(absent) return 0xFF;
    if(response_at<response_n) return response[response_at++];
    if(writing) {
        if(!write_n && value!=0xFE) return 0xFF;
        if(write_n>=1 && write_n<=512) storage[write_n-1]=value;
        if(++write_n==515) {
            writing=false; response_n=response_at=0;respond(reject_write?0x0D:5);respond(0);respond(0xFF);
        }
        return 0xFF;
    }
    if(!packet_n && (value&0xC0)!=0x40) return 0xFF;
    packet[packet_n++]=value;
    if(packet_n==6) {packet_n=0;process_command();}
    return 0xFF;
}
#include "sd_card.c"
int main(void) {
    uint8_t b[512];for(unsigned i=0;i<512;i++) b[i]=(uint8_t)i;
    assert(sd_init());assert(sd_sector_count==131072);assert(clocks_high>=10);assert(fast_calls==1);
    assert(sd_write_sector(15,b));assert(last_address==15);assert(commands[13]==1);
    memset(b,0,512);assert(sd_read_sector(15,b));assert(last_address==15);
    for(unsigned i=0;i<512;i++) assert(b[i]==(uint8_t)i);
    assert(sd_sync()); assert(!sd_read_sector(sd_sector_count,b));assert(sd_error==SD_RANGE);
    hc=false;assert(sd_init());assert(sd_sector_count==524288);
    assert(sd_read_sector(17,b));assert(last_address==17*512);
    hc=true;bad_echo=true;assert(!sd_init());assert(sd_error==SD_UNSUPPORTED);bad_echo=false;
    init_busy=true;assert(!sd_init());assert(sd_error==SD_TIMEOUT);init_busy=false;
    absent=true;assert(!sd_init());assert(!sd_is_ready());absent=false;
    assert(sd_init());bad_token=true;assert(!sd_read_sector(0,b));assert(!sd_is_ready());bad_token=false;
    assert(sd_init());reject_write=true;assert(!sd_write_sector(0,b));assert(!sd_is_ready());reject_write=false;
    assert(!selected);
    puts("PASS SD: SDHC/SDSC addressing, CRC init, OCR/CSD, CMD13, read/write, bounds, missing/busy/rejected card");
    return 0;
}
