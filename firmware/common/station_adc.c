#include "station_adc.h"

static unsigned int8 station_adc_state=0, station_adc_channel=0;
static int1 station_adc_valid=FALSE;
static unsigned int16 station_adc_samples[2];
static unsigned int32 station_adc_started_ms;

void station_adc_init(void)
{
   output_float(AUX_ADC);
   output_float(VIN_ADC);
   setup_adc_ports(sAN0 | sAN1,NO_ANALOGS_P2,VSS_VDD);
   // 64 MHz /64 => TAD=1 us; acquisition handled in task.
   setup_adc(ADC_CLOCK_DIV_64 | ADC_TAD_MUL_0);
   station_adc_state=0;
   station_adc_valid=FALSE;
   station_adc_samples[0]=0;
   station_adc_samples[1]=0;
}

int1 station_adc_start(void)
{
   if(station_adc_state) return FALSE;
   station_adc_channel=STATION_ADC_AUX;
   set_adc_channel(station_adc_channel);
   station_adc_started_ms=protolink_millis();
   station_adc_valid=FALSE;
   station_adc_state=1;
   return TRUE;
}

void station_adc_task(void)
{
   if(station_adc_state==1)
   {
      // >=1 ms acquisition even across tick boundaries, after EVERY mux change.
      if((unsigned int32)(protolink_millis()-station_adc_started_ms)<2) return;
      read_adc(ADC_START_ONLY);
      station_adc_state=2;
   }
   else if(station_adc_state==2 && adc_done())
   {
      station_adc_samples[station_adc_channel]=read_adc(ADC_READ_ONLY);
      if(station_adc_channel==STATION_ADC_AUX)
      {
         station_adc_channel=STATION_ADC_VIN;
         set_adc_channel(station_adc_channel);
         station_adc_started_ms=protolink_millis();
         station_adc_state=1;
      }
      else
      {
         station_adc_valid=TRUE;
         station_adc_state=0;
      }
   }
}

int1 station_adc_ready(void) { return station_adc_valid; }
unsigned int16 station_adc_raw(unsigned int8 channel)
{
   if(channel>STATION_ADC_VIN) return 0;
   return station_adc_samples[channel];
}
int1 station_adc_saturated(unsigned int8 channel)
{
   return station_adc_valid && station_adc_raw(channel)==1023;
}
unsigned int32 station_adc_millivolts(unsigned int8 channel)
{
   unsigned int32 full_scale_mv;
   if(channel>STATION_ADC_VIN) return 0;
   if(channel==STATION_ADC_VIN)
      full_scale_mv=(unsigned int32)STATION_ADC_VREF_MV *
         (STATION_ADC_VIN_R1_KOHM+STATION_ADC_VIN_R2_KOHM)/STATION_ADC_VIN_R2_KOHM;
   else
      full_scale_mv=(unsigned int32)STATION_ADC_VREF_MV *
         (STATION_ADC_AUX_R1_KOHM+STATION_ADC_AUX_R2_KOHM)/STATION_ADC_AUX_R2_KOHM;
   return ((unsigned int32)station_adc_samples[channel]*full_scale_mv+511)/1023;
}
