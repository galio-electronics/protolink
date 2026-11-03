/* Plantilla cliente: LCD opcional y reglas propias despues del init de placa. */
#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#ifdef APP_USE_LCD
#pin_select SCL1IN=PIN_SCL
#pin_select SDA1IN=PIN_SDA
#pin_select SCL1OUT=PIN_SCL
#pin_select SDA1OUT=PIN_SDA
// PCF8574: bus standard-mode de 100 kHz.
#use i2c(Master,I2C1,SLOW,STREAM=LCD_BUS)
#define LCD_I2C_ADDR (LCD_I2C_ADDR_7BIT << 1)
#define LCD_I2C_START() i2c_start(LCD_BUS)
#define LCD_I2C_WRITE(value) i2c_write(LCD_BUS,value)
#define LCD_I2C_STOP() i2c_stop(LCD_BUS)
#endif
#include "../../common/library_config.h"
#include "client_rules.c"

void app_init(void)
{
#ifdef APP_USE_LCD
   int1 lcd_present;
   // protolink_io_init() ya dejo RC3/RC4 digitales y como entradas (I2C).
   i2c_start(LCD_BUS);
   lcd_present = (i2c_write(LCD_BUS,LCD_I2C_ADDR) == 0);
   i2c_stop(LCD_BUS);
   if(lcd_present)
   {
      lcd_init();
      // El nombre completo no cabe en una fila de 16: usar ambas filas.
      lcd_print_xy_ram(1,1,"Galio");
      lcd_print_xy_ram(1,2,"Electronics");
      printf(diagnostics_putc,"INFO lcd_addr=0x%X ack=1 message=Galio_Electronics\r\n",LCD_I2C_ADDR_7BIT);
   }
   else
      printf(diagnostics_putc,"WARN lcd_addr=0x%X ack=0 check_address_wiring\r\n",LCD_I2C_ADDR_7BIT);
#endif
   client_rules_init();
}
void app_task_10ms(unsigned int8 inputs_raw) { client_rules_task(inputs_raw); }
void app_task_100ms(void) { }
void app_task_1000ms(void) { }

#include "../../common/station_app.c"
