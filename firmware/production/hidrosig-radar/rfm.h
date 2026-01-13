/* 
 * File:   rfm.h
 * Author: artga
 *
 * Created on 18 de diciembre de 2025, 10:30 AM
 */

#ifndef RFM_H
#define	RFM_H

#include "../../../libs/simple_master/simple_master.h"

#ifndef RFM_ID
 #define RFM_ID               0x01
#endif


 //RFM VARIABLES
#define RFM_WATER_VOLUME_H              0x03E8
#define RFM_WATER_VOLUME_L              0x03E9
#define RFM_FLOW                        0x03EA
#define RFM_WATER_LEVEL                 0x03EB
#define RFM_FLOW_VELOCITY               0x03EC

//RFM INFO
#define RFM_INFO_SUPPLY_VOLTAGE         0x03EF
#define RFM_INFO_LEVEL_SIGNAL           0x03F0
#define RFM_INFO_VELOCITY_SIGNAL        0x03F1
#define RFM_INFO_CURRENT_METER_ANGLE    0x03F2   
#define RFM_INFO_SW_VERSION             0x03F3

//RFM WORKING MODE
#define RFM_WORKING_MODE                0x03FE
#define RFM_INTERVALS                   0x03FF
#define RFM_WATER_LEVEL_AIR_DRAFT       0x0401
#define RFM_CORRECTION_FACTOR_K         0x0402
#define RFM_CORRECTION_FACTOR_B         0x0403

//RFM ACQUISITION
#define RFM_ACQ_NUM_FLOW_VEL            0x0406
#define RFM_ACQ_FAIL_NUM_FLOW_VEL       0x0407
#define RFM_ACQ_WAIT_TIME_FLOW_VEL      0x0408
#define RFM_ACQ_NUM_WATER_LEVEL         0x0409
#define RFM_ACQ_FAIL_NUM_WATER_LEVEL    0x040A
#define RFM_ACQ_WAIT_TIME_WATER_LEVEL   0x040B

//RFM CROSS SECTION
#define RFM_CS_TYPE                     0x0412
#define RFM_CS_DIMMENSION_1             0x0413
#define RFM_CS_DIMMENSION_2             0x0414
#define RFM_CS_DIMMENSION_3             0x0415

//RFM THRESHOLD
#define RFM_TH_LEVEL                    0x0417
#define RFM_TH_VELOCITY                 0x0418

//MISCELLANEOUS
#define RFM_WATER_LVEL_SMOTHING         0x041A
#define RFM_FLOW_VEL_SMOTHING           0x041B
struct rfm_data{
    int32 water_volume;
    int16 flow;
    int16 water_level;
    int16 flow_velocity;
    int16 data[50];
}radar;

smodbus_status_t rfm_status;

boolean radar_update_all_data(void);

boolean radar_update_all_data(void){
    rfm_status = smodbus_read_holding(RFM_ID,RFM_WATER_VOLUME_H,5,&radar.data);
    if (rfm_status != SMODBUS_OK) {
        return false;

    }
    radar.water_volume = make32(radar.data[0],radar.data[1]);
    radar.flow = radar.data[2];
    radar.water_level = radar.data[3];
    radar.flow_velocity = radar.data[4];
    
    return true;

}

#endif	/* RFM_H */

