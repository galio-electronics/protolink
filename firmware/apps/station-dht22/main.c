#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#pin_select SCL1IN=PIN_SCL
#pin_select SDA1IN=PIN_SDA
#pin_select SCL1OUT=PIN_SCL
#pin_select SDA1OUT=PIN_SDA
#use i2c(Master,I2C1,SLOW,STREAM=LCD_BUS)
#define LCD_I2C_ADDR (LCD_I2C_ADDR_7BIT << 1)
#define LCD_I2C_START() i2c_start(LCD_BUS)
#define LCD_I2C_WRITE(value) i2c_write(LCD_BUS,value)
#define LCD_I2C_STOP() i2c_stop(LCD_BUS)
#include "../../common/library_config.h"
#include "../../common/dht22.c"
#include "ethernet_json.c"

static int1 display_present = FALSE;
static char display_lines[2][17];
static unsigned int8 display_cursor = 32;
static unsigned int32 last_sample_ms;

void display_set(unsigned int8 row, char *text)
{
   unsigned int8 i;
   int1 ended = FALSE;
   for(i=0;i<16;i++)
   {
      if(!ended && !text[i]) ended = TRUE;
      display_lines[row][i] = ended ? ' ' : text[i];
   }
   display_lines[row][16] = 0;
   display_cursor = 0;
}

void display_task(void)
{
   unsigned int8 row, col;
   if(!display_present || display_cursor >= 32 || dht22_busy()) return;
   row = display_cursor / 16;
   col = display_cursor % 16;
   if(col == 0) lcd_gotoxy(1,row+1);
   lcd_putc(display_lines[row][col]);
   ++display_cursor;
}

void app_init(void)
{
   ethernet_init();
   dht22_init();
   i2c_start(LCD_BUS);
   display_present = (i2c_write(LCD_BUS,LCD_I2C_ADDR) == 0);
   i2c_stop(LCD_BUS);
   if(display_present) lcd_init();
   display_set(0,"DHT22 / Ethernet");
   display_set(1,"Esperando...");
   last_sample_ms = protolink_millis();
   printf(diagnostics_putc,"INFO dht=RA2 eth=UART3 baud=115200 lcd=%u\r\n",display_present);
}

void app_task(void)
{
   dht22_task();
}

void app_task_10ms(unsigned int8 inputs_raw)
{
   unsigned int8 result;
   unsigned int16 magnitude;
   unsigned int32 now = protolink_millis();
   char temperature[9], humidity[8], line[17];
   int1 published;
   result = dht22_result();
   if(result == DHT22_OK)
   {
      if(dht22_temperature_x10 < 0)
      {
         magnitude = -dht22_temperature_x10;
         sprintf(temperature,"-%LU.%u",(unsigned int32)(magnitude/10),(unsigned int8)(magnitude%10));
      }
      else
      {
         magnitude = dht22_temperature_x10;
         sprintf(temperature,"%LU.%u",(unsigned int32)(magnitude/10),(unsigned int8)(magnitude%10));
      }
      sprintf(humidity,"%LU.%u",(unsigned int32)(dht22_humidity_x10/10),(unsigned int8)(dht22_humidity_x10%10));
      sprintf(line,"Temp: %s C",temperature);
      display_set(0,line);
      sprintf(line,"Hum:  %s %%",humidity);
      display_set(1,line);
      published = ethernet_publish(temperature,humidity);
      printf(diagnostics_putc,"SENSOR temperature=%s humidity=%s uart3_queued=%u\r\n",temperature,humidity,published);
   }
   else if(result != DHT22_PENDING)
   {
      display_set(0,"DHT22 error");
      sprintf(line,"Codigo: %u",result);
      display_set(1,line);
      printf(diagnostics_putc,"WARN dht22_error=%u edges=%u level=%u start=%u/%u\r\n",result,dht22_edge_count,input(DI_1WIRE),dht22_start_low,dht22_start_edges);
   }
   if((unsigned int32)(now-last_sample_ms) >= DHT22_SAMPLE_INTERVAL_MS && !dht22_busy())
   {
      last_sample_ms = now;
      dht22_start();
   }
   display_task();
}
void app_task_100ms(void) { }
void app_task_1000ms(void) { }
#include "../../common/station_app.c"


