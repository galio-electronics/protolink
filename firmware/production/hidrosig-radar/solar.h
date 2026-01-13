/* 
 * File:   solar.h
 * Author: artga
 *
 * Created on 19 de diciembre de 2025, 08:47 AM
 */

#ifndef SOLAR_H
#define	SOLAR_H
#include "../../../libs/simple_master/simple_master.h"

#ifndef SOLAR_ID
 #define SOLAR_ID               0x01
#endif

#define SOLAR_PV_VOLTAGE        0x3100
#define SOLAR_PV_CURRENT        0x3101
#define SOLAR_PV_PWR_L          0x3102
#define SOLAR_PV_PWR_H          0x3103

#define SOLAR_BAT_PWR_L         0x3106
#define SOLAR_BAT_PWR_H         0x3107

#define SOLAR_LOAD_VOLTAGE      0x310C
#define SOLAR_LOAD_CURRENT      0x310D
#define SOLAR_LOAD_PWR_L        0x310E
#define SOLAR_LOAD_PWR_H        0x310F

#define SOLAR_BAT_TEMPERATURE   0x3110
#define SOLAR_TEMP_INSIDE       0x3111

struct solar_data{
    int16 panel_voltage_raw;
    int16 panel_current_raw;
    int32 panel_pwr_raw;
    int32 bat_pwr_raw;
    int16 load_voltage_raw;
    int16 load_current_raw;
    int32 load_pwr_raw;
    int16 bat_temperature;
    int16 temperaure_inside;
    int16 data[50];
}solar;

smodbus_status_t solar_status;

boolean solar_update_all_data(void);

boolean solar_update_all_data(void){
    solar_status = smodbus_read_input(SOLAR_ID,SOLAR_PV_VOLTAGE,19,&solar.data);
    if (solar_status != SMODBUS_OK) {        
        return false;
    }
    solar.panel_voltage_raw = solar.data[0];
    solar.panel_current_raw = solar.data[1];
    solar.panel_pwr_raw = make32(solar.data[2],solar.data[3]);
    solar.bat_pwr_raw = make32(solar.data[6],solar.data[7]);
    solar.load_voltage_raw = solar.data[12] ;
    solar.load_current_raw = solar.data[13];
    solar.load_pwr_raw = make32(solar.data[14],solar.data[15]);
    solar.bat_temperature = solar.data[16];
    solar.temperaure_inside = solar.data[17];

    return true;
}

#endif	/* SOLAR_H */

