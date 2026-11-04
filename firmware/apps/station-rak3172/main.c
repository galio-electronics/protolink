#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#pin_select U5TX=TX_5
#pin_select U5RX=RX_5
#pin_select SCL1IN=PIN_SCL
#pin_select SDA1IN=PIN_SDA
#pin_select SCL1OUT=PIN_SCL
#pin_select SDA1OUT=PIN_SDA
#use i2c(Master,I2C1,FAST=400000,STREAM=OLED_I2C)
#define OLED_I2C_STREAM OLED_I2C
#include "../../common/library_config.h"
#include "../../common/dht22.c"

/* Adapter for the existing driver: reuse UART/ISR/ring and TX, but parse
 * responses cooperatively. Its blocking API discards asynchronous events.
 * INT_RDA5 is masked only during ring pop (16-bit indices on an 8-bit PIC).
 * No calls to rui3_at_task/cmd may be mixed with this adapter.
 */
static char response[128];
static unsigned int8 response_len=0, step=0;
static int1 overflow=FALSE, pending=FALSE, joined=FALSE, join_wait=FALSE;
static int1 tx_wait=FALSE;
static unsigned int32 command_ms, next_ms=2000, radio_ms, sequence=0;
static char command[80];
static unsigned int8 sensor_status=DHT22_PENDING;
static unsigned int32 sample_ms=0;
static signed int16 temperature_x10=0;
static unsigned int16 humidity_x10=0;
static int1 display_present=FALSE;
static char display_lines[3][22];
static unsigned int8 display_cursor=63;

// Pad complete lines so shorter values cannot leave old digits visible.
void demo_display_set(unsigned int8 row, char *text)
{
   unsigned int8 i;
   int1 ended=FALSE;
   for(i=0;i<21;i++)
   {
      if(!ended && !text[i]) ended=TRUE;
      display_lines[row][i]=ended ? ' ' : text[i];
   }
   display_lines[row][21]=0;
}

void demo_display_update(void)
{
   char line[22];
   unsigned int16 magnitude;
   if(sensor_status==DHT22_OK)
   {
      magnitude=temperature_x10<0 ? -temperature_x10 : temperature_x10;
      sprintf(line,"Temp: %c%LU.%u C",temperature_x10<0 ? '-' : ' ',
         (unsigned int32)(magnitude/10),(unsigned int8)(magnitude%10));
      demo_display_set(0,line);
      sprintf(line,"Humedad: %LU.%u %%",(unsigned int32)(humidity_x10/10),
         (unsigned int8)(humidity_x10%10));
      demo_display_set(1,line);
      demo_display_set(2,"DHT22 OK");
   }
   else
   {
      demo_display_set(0,"Temp: --.- C");
      demo_display_set(1,"Humedad: --.- %");
      if(sensor_status==DHT22_PENDING)
         demo_display_set(2,"Sin dato reciente");
      else
      {
         sprintf(line,"Error DHT22: %u",sensor_status);
         demo_display_set(2,line);
      }
   }
   display_cursor=0;
}

void demo_display_task(void)
{
   unsigned int8 row, col;
   if(!display_present || display_cursor>=63 || dht22_busy()) return;
   row=display_cursor/21;
   col=display_cursor%21;
   if(col==0) oled_set_pos(0,2+row*2);
   oled_putc(display_lines[row][col]);
   ++display_cursor;
}

void demo_send(char *text)
{
   pending=TRUE;
   command_ms=protolink_millis();
   _rui3_at_send_line(text);
}

void demo_line(void)
{
   unsigned int32 now=protolink_millis();
   if(!strcmp(response,"+EVT:JOINED"))
   {
      joined=TRUE; join_wait=FALSE; next_ms=now+2000;
      printf(diagnostics_putc,"LORA JOINED\r\n");
   }
   else if(!strncmp(response,"+EVT:JOIN_FAILED",16) ||
           !strncmp(response,"+EVT:JOIN FAILED",16))
   {
      joined=FALSE; join_wait=FALSE; next_ms=now+DEMO_INTERVAL_MS;
      printf(diagnostics_putc,"LORA JOIN_FAILED retry=60s\r\n");
   }
   else if(!strcmp(response,"+EVT:TX_DONE"))
   {
      tx_wait=FALSE;
      printf(diagnostics_putc,"LORA TX_DONE\r\n");
   }
   else if(!strncmp(response,"+EVT:RX_",8))
      printf(diagnostics_putc,"LORA %s\r\n",response);
   else if(!strcmp(response,"OK") && pending)
   {
      pending=FALSE;
      if(step==0)
         printf(diagnostics_putc,"INFO RAK_DETECTADO UART5: AT OK\r\n");
      if(step<14) { ++step; next_ms=now+200; }
   }
   else if(!strncmp(response,"AT_",3))
   {
      printf(diagnostics_putc,"LORA error step=%u %s\r\n",step,response);
      pending=FALSE; join_wait=FALSE; tx_wait=FALSE;
      if(!strcmp(response,"AT_NO_NETWORK_JOINED")) joined=FALSE;
      next_ms=now+DEMO_INTERVAL_MS;
   }
   else if(pending && (step==1 || step==2 || step==12 || step==13))
   {
      // Ignore AT command echoes; never print credentials or unknown lines.
      if(response[strlen(response)-1]!='?')
         printf(diagnostics_putc,"LORA query=%u %s\r\n",step,response);
   }
}

void app_init(void)
{
   output_high(TX_5);
   set_tris_e(0b11111011); // RE2 TX5; RE3 RX5; DI1..4 remain inputs.
   setup_uart(RUI3_AT_BAUD,RUI3_UART);
   rui3_at_init();
   dht22_init();
   i2c_start(OLED_I2C);
   display_present=(i2c_write(OLED_I2C,OLED_I2C_ADDR_WRITE)==0);
   i2c_stop(OLED_I2C);
   if(display_present)
   {
      oled_init();
      oled_print_xy(0,0,"RAK3172 / DHT22");
   }
   demo_display_update();
   printf(diagnostics_putc,"INFO OLED SSD1306 I2C1 0x3C presente=%u\r\n",display_present);
   printf(diagnostics_putc,"LORA UART5 RE2/RE3 115200 US915 channels=0-7\r\n");
}

void app_task(void)
{
   unsigned int8 c, budget=32;
   int1 available;
   dht22_task();
   while(budget--)
   {
      disable_interrupts(INT_RDA5);
      available=_rx_ring_pop(&c);
      enable_interrupts(INT_RDA5);
      if(!available) break;
      if(c=='\r') continue;
      if(c=='\n')
      {
         response[response_len]=0;
         if(!overflow && response_len) demo_line();
         response_len=0; overflow=FALSE;
      }
      else if(response_len<sizeof(response)-1) response[response_len++]=c;
      else overflow=TRUE; // Discard entire oversized line, never parse its suffix.
   }
}

void app_task_10ms(unsigned int8 inputs_raw)
{
   unsigned int8 result=dht22_result();
   unsigned int32 now=protolink_millis();
   if(result!=DHT22_PENDING)
   {
      sensor_status=result;
      if(result==DHT22_OK)
      {
         temperature_x10=dht22_temperature_x10;
         humidity_x10=dht22_humidity_x10;
      }
      else { temperature_x10=0; humidity_x10=0; }
      demo_display_update();
      printf(diagnostics_putc,"DHT status=%u temp_x10=%LD humidity_x10=%LU\r\n",
         sensor_status,(signed int32)temperature_x10,(unsigned int32)humidity_x10);
   }
   if(sensor_status==DHT22_OK && (unsigned int32)(now-sample_ms)>15000)
   {
      sensor_status=DHT22_PENDING;
      demo_display_update();
   }
   if((unsigned int32)(now-sample_ms)>=5000 && !dht22_busy() &&
      !pending && !join_wait && !tx_wait)
   {
      sample_ms=now;
      dht22_start();
   }
   demo_display_task();
}
void app_task_100ms(void)
{
   unsigned int32 now=protolink_millis();
   // LED3: 500 ms on/off until OTAA succeeds; solid while joined.
   // Update before early returns so retries and sensor capture do not stop it.
   output_bit(LED3,(joined || ((now / 500) & 1)) ==
      ((STATION_LED_ACTIVE_MASK & 4) != 0));
   if(dht22_busy()) return;
   if(pending)
   {
      if((unsigned int32)(now-command_ms)>3000)
      {
         pending=FALSE; join_wait=FALSE; tx_wait=FALSE;
         response_len=0; overflow=FALSE;
         next_ms=now+DEMO_INTERVAL_MS;
         if(step==0)
            printf(diagnostics_putc,"WARN RAK_NO_DETECTADO: sin OK UART5. Revisar alimentacion/socket/TX-RX/baud 115200/modo AT. retry=60s\r\n");
         else
            printf(diagnostics_putc,"WARN RAK_SIN_RESPUESTA step=%u: revisar UART5/alimentacion. Reintento AT en 60s\r\n",step);
         // Recheck communication before configuring or sending again.
         // A silent modem does not prove physical absence or network failure.
         joined=FALSE;
         step=0;
      }
      return;
   }
   if(join_wait || tx_wait)
   {
      if((unsigned int32)(now-radio_ms)>30000)
      {
         join_wait=FALSE; tx_wait=FALSE;
         next_ms=now+DEMO_INTERVAL_MS;
         printf(diagnostics_putc,"LORA EVENT_TIMEOUT retry=60s\r\n");
      }
      return;
   }
   if((signed int32)(now-next_ms)<0) return;
   switch(step)
   {
      case 0: demo_send("AT"); break;
      case 1: demo_send("AT+VER=?"); break;
      case 2: demo_send("AT+DEVEUI=?"); break; // Preserve factory identity.
      case 3: demo_send("AT+NWM=1"); break;
      case 4: demo_send("AT+NJM=1"); break;
      case 5: demo_send("AT+BAND=5"); break;
      case 6: demo_send("AT+MASK=0001"); break;
      case 7: sprintf(command,"AT+APPEUI=%s",DEMO_JOIN_EUI); demo_send(command); break;
      case 8: sprintf(command,"AT+APPKEY=%s",DEMO_APP_KEY); demo_send(command); break;
      case 9: demo_send("AT+CLASS=A"); break;
      case 10: demo_send("AT+ADR=1"); break;
      case 11: demo_send("AT+CFM=0"); break;
      case 12: demo_send("AT+BAND=?"); break;
      case 13: demo_send("AT+MASK=?"); break;
      default:
         radio_ms=now; next_ms=now+DEMO_INTERVAL_MS;
         if(!joined)
         {
            join_wait=TRUE;
            demo_send("AT+JOIN=1:0:60:1");
            printf(diagnostics_putc,"LORA JOIN start\r\n");
         }
         else
         {
            // V2: version, DI, sequence32, DHT status, temp16 signed, RH16.
            // Big endian; values in tenths. Never publish stale readings.
            if((unsigned int32)(now-sample_ms)>15000) sensor_status=DHT22_PENDING;
            sprintf(command,"AT+SEND=10:02%X%X%X%X%X%X%X%X%X%X",protolink_read_inputs(),
               (unsigned int8)(sequence>>24),(unsigned int8)(sequence>>16),
               (unsigned int8)(sequence>>8),(unsigned int8)sequence,
               sensor_status,(unsigned int8)((unsigned int16)temperature_x10>>8),
               (unsigned int8)temperature_x10,(unsigned int8)(humidity_x10>>8),
               (unsigned int8)humidity_x10);
            tx_wait=TRUE; demo_send(command);
            printf(diagnostics_putc,"LORA uplink seq=%LU port=10\r\n",sequence);
            ++sequence;
         }
         break;
   }
}
void app_task_1000ms(void) { }
#include "../../common/station_app.c"
