/* PIC18F67K40 MSSP2, dedicated to SD. Timer2 remains the scheduler. */
#pin_select SCK2IN=SPI_SCK
#pin_select SCK2OUT=SPI_SCK
#pin_select SDI2=SPI_MISO
#pin_select SDO2=SPI_MOSI
#byte sd_sspbuf = getenv("SFR:SSP2BUF")
#byte sd_sspstat = getenv("SFR:SSP2STAT")
#byte sd_sspcon = getenv("SFR:SSP2CON1")
#byte sd_sspadd = getenv("SFR:SSP2ADD")
#byte sd_trisd = getenv("SFR:TRISD")
static int1 sd_port_fault = FALSE;
#define SD_MILLIS() protolink_millis()
#if (SD_RUN_HZ < 62500) || (SD_RUN_HZ > 8000000)
#error SD_RUN_HZ fuera del rango de SSP2ADD
#endif
#if (16000000 % SD_RUN_HZ) != 0
#error SD_RUN_HZ debe dividir exactamente 16000000
#endif
static void sd_port_clock(unsigned int8 divisor)
{
   sd_sspcon=0;
   sd_sspstat=0x40; // SMP=0, CKE=1, CKP=0: SPI mode 0.
   sd_sspadd=divisor;
   sd_sspcon=0x2A; // SSPEN + SSPM=1010, Fosc/(4*(SSP2ADD+1)).
}
void sd_port_select(int1 active) { output_bit(SPI_CS,!active); }
void sd_port_init(void)
{
   sd_port_fault = FALSE;
   output_high(SPI_CS); output_high(SPI_MOSI); output_low(SPI_SCK);
   /* protolink_io_init has explicitly disabled all analog functions. */
   sd_trisd = (sd_trisd & 0x0F) | 0x20;
   /* CCS 5.081 spi_init(400000) selects Fosc/64=1 MHz at 64 MHz.
    * Use the documented BRG mode instead (DS40001841G, page 522). */
   sd_port_clock(39);
}
void sd_port_fast(void) { sd_port_clock(16000000UL/SD_RUN_HZ-1); }
unsigned int8 sd_port_xfer(unsigned int8 value)
{
   unsigned int16 timeout = 1000;
   if(sd_port_fault) return 0xFF;
   sd_sspbuf = value;
   while(!(sd_sspstat & 1) && --timeout) delay_us(1);
   if(!timeout || (sd_sspcon & 0xC0)) {
      sd_port_fault = TRUE;
      return 0xFF;
   }
   return sd_sspbuf;
}
