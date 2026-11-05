#ifndef STATION_I2C_C
#define STATION_I2C_C
/* Dedicated I2C1 adapter. Same PPS as existing Station LCD demos, but
 * bounded register polling so disconnected/stuck buses cannot hang main. */
#pin_select SCL1IN=PIN_SCL
#pin_select SDA1IN=PIN_SDA
#pin_select SCL1OUT=PIN_SCL
#pin_select SDA1OUT=PIN_SDA
#byte rtc_con1 = getenv("SFR:SSP1CON1")
#byte rtc_con2 = getenv("SFR:SSP1CON2")
#byte rtc_stat = getenv("SFR:SSP1STAT")
#byte rtc_buf = getenv("SFR:SSP1BUF")
#byte rtc_add = getenv("SFR:SSP1ADD")
#byte rtc_tris = getenv("SFR:TRISC")
/* Last transaction evidence: stage 1=start,2=device,3=address-high,
 * 4=register/address-low,5=data,6=restart/read-address; error 1=lines,
 * 2=controller/timeout,3=NACK. Main-only diagnostics. */
unsigned int8 station_i2c_stage=0,station_i2c_error=0;
void rtc_bus_init(void)
{
   rtc_con1=0; rtc_con2=0;
   /* protolink_io_init has explicitly disabled all analog functions. */
   rtc_tris |= 0x18;
   rtc_stat=0x80; rtc_add=159; rtc_con1=0x28; // 64 MHz / (4*160) = 100 kHz.
}
static int1 rtc_bus_idle(void)
{
   unsigned int16 timeout=1000;
   while(((rtc_con2 & 0x1F) || (rtc_stat & 4)) && --timeout) delay_us(2);
   return timeout!=0 && !(rtc_con1 & 0xC0);
}
static int1 rtc_bus_end(int1 ok)
{
   if(ok) {
      rtc_con2 |= 4;
      ok=rtc_bus_idle();
   }
   if(!ok) rtc_bus_init();
   return ok;
}
static int1 rtc_bus_tx(unsigned int8 v)
{
   rtc_buf=v;
   if(!rtc_bus_idle()) { station_i2c_error=2; return FALSE; }
   if(rtc_con2 & 0x40) { station_i2c_error=3; return FALSE; }
   return TRUE;
}
/* Shared single-owner bus: all transactions complete in main, never an ISR. */
static int1 station_i2c_address(unsigned int8 device, unsigned int16 address, unsigned int8 width)
{
   station_i2c_error=0; station_i2c_stage=1;
   if(!input(PIN_SCL) || !input(PIN_SDA)) { station_i2c_error=1; return FALSE; }
   rtc_con2 |= 1;
   if(!rtc_bus_idle()) { station_i2c_error=2; return FALSE; }
   station_i2c_stage=2;
   if(!rtc_bus_tx(device)) return FALSE;
   station_i2c_stage=3;
   if(width==2 && !rtc_bus_tx((unsigned int8)(address>>8))) return FALSE;
   station_i2c_stage=4;
   return rtc_bus_tx((unsigned int8)address);
}
int1 station_i2c_write(unsigned int8 device, unsigned int16 address, unsigned int8 width,
                       unsigned int8 *data, unsigned int8 n)
{
   unsigned int8 i;
   if(!station_i2c_address(device,address,width)) return rtc_bus_end(FALSE);
   station_i2c_stage=5;
   for(i=0;i<n;i++) if(!rtc_bus_tx(data[i])) return rtc_bus_end(FALSE);
   return rtc_bus_end(TRUE);
}
int1 station_i2c_read(unsigned int8 device, unsigned int16 address, unsigned int8 width,
                      unsigned int8 *data, unsigned int8 n)
{
   unsigned int8 i;
   unsigned int16 timeout;
   if(!station_i2c_address(device,address,width)) return rtc_bus_end(FALSE);
   rtc_con2 |= 2;
   station_i2c_stage=6;
   if(!rtc_bus_idle() || !rtc_bus_tx(device | 1)) return rtc_bus_end(FALSE);
   for(i=0;i<n;i++) {
      rtc_con2 |= 8;
      timeout=1000;
      while(!(rtc_stat & 1) && --timeout) delay_us(2);
      if(!timeout) return rtc_bus_end(FALSE);
      data[i]=rtc_buf;
      if(i==n-1) rtc_con2 |= 0x20; else rtc_con2 &= ~0x20;
      rtc_con2 |= 0x10;
      if(!rtc_bus_idle()) return rtc_bus_end(FALSE);
   }
   return rtc_bus_end(TRUE);
}
int1 station_i2c_probe(unsigned int8 device)
{
   int1 ok;
   station_i2c_error=0; station_i2c_stage=1;
   if(!input(PIN_SCL) || !input(PIN_SDA)) { station_i2c_error=1; return FALSE; }
   rtc_con2 |= 1;
   station_i2c_stage=2;
   ok=rtc_bus_idle() && rtc_bus_tx(device);
   /* NACK is normal while EEPROM programs; still send STOP. */
   return rtc_bus_end(TRUE) && ok;
}

#endif
