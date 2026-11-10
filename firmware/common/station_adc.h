#ifndef PROTOLINK_STATION_ADC_H
#define PROTOLINK_STATION_ADC_H
// PIC18F67K40, ADC=10, Fosc=64 MHz. Owns ADC and analog port setup.
// Main only, after Timer2 init. Do not mix with other ADC drivers.
#define STATION_ADC_AUX 0
#define STATION_ADC_VIN 1
#ifndef STATION_ADC_VREF_MV
#error Define STATION_ADC_VREF_MV as measured VDD in millivolts
#endif
#ifndef STATION_ADC_VIN_R1_KOHM
#define STATION_ADC_VIN_R1_KOHM 100UL
#endif
#ifndef STATION_ADC_VIN_R2_KOHM
#define STATION_ADC_VIN_R2_KOHM 10UL
#endif
#ifndef STATION_ADC_AUX_R1_KOHM
#define STATION_ADC_AUX_R1_KOHM 10UL
#endif
#ifndef STATION_ADC_AUX_R2_KOHM
#define STATION_ADC_AUX_R2_KOHM 10UL
#endif
#if (STATION_ADC_VIN_R2_KOHM == 0) || (STATION_ADC_AUX_R2_KOHM == 0)
#error ADC divider R2 must be positive
#endif
void station_adc_init(void);
int1 station_adc_start(void); // Starts AUX then VIN; FALSE if busy.
void station_adc_task(void);  // Call frequently, no blocking acquisition.
int1 station_adc_ready(void); // Both results valid until next accepted start.
// Read only when ready. Invalid channel returns zero/FALSE.
unsigned int16 station_adc_raw(unsigned int8 channel);
unsigned int32 station_adc_millivolts(unsigned int8 channel);
int1 station_adc_saturated(unsigned int8 channel);
#endif
