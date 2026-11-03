/* UART3 JSON+CRLF for the existing Ethernet serial/MQTT bridge. */
#pin_select U3TX=TX_3
#pin_select U3RX=RX_3
#use rs232(baud=ETHERNET_BAUD,parity=N,UART3,bits=8,stream=ETHERNET,errors)
static char ethernet_buffer[240];
static unsigned int8 ethernet_length;
volatile unsigned int8 ethernet_position=0;
volatile int1 ethernet_sending=FALSE;
#INT_TBE3
void ethernet_tx_isr(void)
{
   if(ethernet_position<ethernet_length)
      fputc(ethernet_buffer[ethernet_position++],ETHERNET);
   else { disable_interrupts(INT_TBE3); ethernet_sending=FALSE; }
}
void ethernet_init(void)
{
   disable_interrupts(INT_TBE3);
   output_high(TX_3);
   set_tris_e(0b11111110);
   setup_uart(ETHERNET_BAUD,ETHERNET);
}
void ethernet_start(void)
{
   ethernet_length=strlen(ethernet_buffer);
   ethernet_position=0;
   ethernet_sending=TRUE;
   enable_interrupts(INT_TBE3);
}
