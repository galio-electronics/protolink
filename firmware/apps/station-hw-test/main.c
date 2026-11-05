#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#include "../../common/event_station.c"
#include "../../common/event_health.c"
#include "../station-rtc-sd/rtc_port.c"
#include "../../common/mcp79402.h"
#include "../../common/event_rtc.c"
#include "../../common/mcp79402.c"
/* Reuse the existing ADS register definitions. Its blocking conversion API
 * has no deadline or ACK propagation, so this test uses the checked shared
 * bus adapter and a cooperative conversion sequence instead. */
#define ADS1115_I2C_ADDR_7BIT 0x49
#include <analog_drivers/ads1115.h>
static unsigned int8 hw_phase=0,hw_channel=0,hw_result=255,hw_rdy,hw_rdy_busy;
static signed int16 hw_raw[2];
static unsigned int32 hw_started=0,hw_last=0,hw_samples=0,hw_errors=0;
static unsigned int16 hw_cfg;
static int1 hw_ready_setup=FALSE,hw_announce=TRUE;
static unsigned int8 hw_probe=255;
int1 hw_command(char *command)
{
   if(!strcmp(command,"ADC PROBE")) { hw_probe=0; return TRUE; }
   return FALSE;
}
static int1 hw_adc_read(unsigned int8 reg,unsigned int16 *value)
{
   unsigned int8 data[2];
   if(!station_i2c_read(ADS1115_ADDR_WRITE,reg,1,data,2)) return FALSE;
   *value=((unsigned int16)data[0]<<8)|data[1]; return TRUE;
}
static int1 hw_adc_write(unsigned int8 reg,unsigned int16 value)
{
   unsigned int8 data[2];
   data[0]=value>>8; data[1]=value;
   return station_i2c_write(ADS1115_ADDR_WRITE,reg,1,data,2);
}
static void hw_finish(unsigned int8 result)
{
   hw_result=result; hw_phase=0; hw_last=protolink_millis();
   if(result) { ++hw_errors; hw_ready_setup=FALSE; }
   else ++hw_samples;
   event_health_adc(result==0,result,hw_channel);
}
void app_init(void)
{
   rtc_init();
}
void app_task(void)
{
   unsigned int32 now=protolink_millis();
   unsigned int16 value,expected;
   if(hw_phase==0) {
      if((unsigned int32)(now-hw_last)<1000 || hw_result!=255) return;
      hw_channel^=1;
      if(!hw_ready_setup) {
         /* TI conversion-ready mode: Hi_thresh MSB=1, Lo_thresh MSB=0. */
         if(!hw_adc_write(ADS1115_REG_LO_THRESH,0) ||
            !hw_adc_write(ADS1115_REG_HI_THRESH,0x8000)) { hw_finish(1); return; }
         hw_ready_setup=TRUE;
      }
      expected=ADS1115_OS_SINGLE_START | ADS1115_MODE_SINGLE |
         ADS1115_PGA_6_144V | ADS1115_DR_128SPS;
      expected |= hw_channel ? ADS1115_MUX_A1_GND : ADS1115_MUX_A0_GND;
      hw_cfg=expected;
      if(!hw_adc_write(ADS1115_REG_CONFIG,expected)) { hw_finish(1); return; }
      hw_rdy_busy=input(ADC_RDY);
      hw_started=now; hw_phase=1;
   } else {
      /* Single-shot RDY follows conversion status; reading data need not
       * release it. Observe high during conversion, low when complete. */
      if(input(ADC_RDY)) hw_rdy_busy=1;
      if((unsigned int32)(now-hw_started)<10) return;
      if(!hw_adc_read(ADS1115_REG_CONFIG,&value)) { hw_finish(2); return; }
      if((value & 0x7FFF)!=(hw_cfg & 0x7FFF)) { hw_finish(3); return; }
      if(!(value & ADS1115_OS_MASK)) {
         if((unsigned int32)(now-hw_started)>=100) hw_finish(4);
         return;
      }
      hw_rdy=input(ADC_RDY);
      if(!hw_adc_read(ADS1115_REG_CONVERSION,&value)) { hw_finish(2); return; }
      hw_raw[hw_channel]=(signed int16)value;
      hw_finish(0);
   }
}
void app_task_10ms(unsigned int8 inputs_raw)
{
   unsigned int8 raw[2],address;
   int1 ok,ack;
   if(hw_probe<4) {
      address=0x48+hw_probe++;
      ack=station_i2c_probe(address<<1);
      ok=station_i2c_read(address<<1,ADS1115_REG_CONFIG,1,raw,2);
      printf(diagnostics_putc,"SONDA ADC 0x%02X ACK=%u lectura=%u cfg=%02X%02X etapa=%u error=%u SCL=%u SDA=%u\r\n",
         address,ack,ok,ok ? raw[0] : 0,ok ? raw[1] : 0,station_i2c_stage,station_i2c_error,input(PIN_SCL),input(PIN_SDA));
      return;
   }
   if(hw_announce) {
      printf(diagnostics_putc,"PRUEBA: ADS1115 addr=0x49 RDY=RA3 AIN0/AIN1; raw sin calibracion 4-20mA.\r\n");
      hw_announce=FALSE; return;
   }
   if(hw_result==255) return;
   if(hw_result==0)
      printf(diagnostics_putc,"ADC OK AIN%u raw=%Ld cfg=%04LX RDY_ocupado=%u listo=%u n=%LU errores=%LU\r\n",
         hw_channel,hw_raw[hw_channel],hw_cfg,hw_rdy_busy,hw_rdy,hw_samples,hw_errors);
   else {
      printf(diagnostics_putc,"ADC FALLO AIN%u: ",hw_channel);
      switch(hw_result) {
         case 1: printf(diagnostics_putc,"NACK/error al configurar"); break;
         case 2: printf(diagnostics_putc,"NACK/error al leer"); break;
         case 3: printf(diagnostics_putc,"configuracion leida no coincide"); break;
         default: printf(diagnostics_putc,"conversion no termino en 100ms"); break;
      }
      printf(diagnostics_putc,"; reintento=1s errores=%LU\r\n",hw_errors);
   }
   hw_result=255;
}
void app_task_100ms(void) { }
void app_task_1000ms(void)
{
   rtc_datetime_t dt;
   static unsigned int8 last_error=255;
   int1 ok=rtc_get_datetime(&dt);
   if(last_error!=rtc_error) {
      if(ok) printf(diagnostics_putc,"RTC comunica y calendario valido; UTC no sincronizado en esta prueba.\r\n");
      else printf(diagnostics_putc,"RTC no confiable: codigo=%u (1=bus 2=fecha 3=oscilador 4=PWRFAIL). Eventos sin UTC.\r\n",rtc_error);
      last_error=rtc_error;
   }
}
#include "../../common/station_app.c"
