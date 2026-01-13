
#include "../../protolink/v1.h"
/*Hardware defines*/
#define COMPILACION_DATE    __DATE__
#define COMPILACION_TIME    __TIME__
#define FW_VERSION  1.0
#define HW_NAME "Protolink Hidrosig"

//TODO:
//Add LoRa libs

// ========== CONFIG SIMPLE-MASTER ==========
#define SMODBUS_DEBUG           0
#define SMODBUS_BAUD          115200
#define SMODBUS_SERIAL_INT    SMODBUS_INT_RDA4      // o MODBUS_INT_RDA2 / MODBUS_INT_EXT
#define SMODBUS_RX_PIN        RX_4              // UART RX
#define SMODBUS_TX_PIN        TX_4              // UART TX
#define SMODBUS_DE_PIN        TX_ENABLE              // DE RS485
#define SMODBUS_RE_PIN        RX_ENABLE                   // 0 si esta amarrado a GND o unido a DE
#define SMODBUS_TIMEOUT_MS   1000   // 1 segundo
#define SMODBUS_GAP_MS       50     // 50 ms de silencio para ?fin de trama?
#define SMODBUS_RX_BUFFER     64   

#define DEBUG_MSG(msg) fprintf(DEBUG, msg)

#define SOLAR_ID      0x02
#define RFM_ID        0x01

#include "rfm.h"
#include "solar.h"
#include "../../../libs/simple_master/simple_master.c"
#include <bootloader.h>
#include <stdio.h>
#include <stdlib.h>




void print_status(smodbus_status_t st) {
   switch(st){
      case SMODBUS_OK:             protolink_debug_msg("OK\r\n"); break;
      case SMODBUS_ERR_TIMEOUT:    protolink_debug_msg("ERR: TIMEOUT\r\n"); break;
      case SMODBUS_ERR_EXCEPTION:  protolink_debug_msg("ERR: EXCEPTION\r\n"); break;
      case SMODBUS_ERR_CRC:        protolink_debug_msg("ERR: CRC\r\n"); break;
      case SMODBUS_ERR_FRAME:      protolink_debug_msg("ERR: FRAME\r\n"); break;
      default:                 protolink_debug_msg("ERR: OTHER\r\n"); break;
   }
}

void main() {
    protolink_io_init();
    smodbus_init();   // Inicializa Modbus

    while(TRUE) {
        output_toggle(LED1);


        delay_ms(1000); // ciclo de pooling simple
        if(radar_update_all_data()){
            protolink_debug_data("[RFM] Flow : %ld\r\n",radar.flow);
            protolink_debug_data("[RFM] Water Level : %ld\r\n",radar.water_level);
            protolink_debug_data("[RFM] Flow Velocity : %ld\r\n",radar.flow_velocity);
        }
        else{
            protolink_debug_msg("[RFM] FAILURE\r\n");
        }
        if(solar_update_all_data()){
            protolink_debug_data("[SOLAR] Voltaje Panel: %ld\r\n",solar.panel_voltage_raw);
            protolink_debug_data("[SOLAR] Current Panel: %ld\r\n",solar.panel_current_raw);
            protolink_debug_data("[SOLAR] Voltage Load: %ld\r\n",solar.load_voltage_raw);
            protolink_debug_data("[SOLAR] Current Load: %ld\r\n",solar.load_current_raw);
            protolink_debug_data("[SOLAR] Temperature: %ld\r\n",solar.temperaure_inside);
        }else{
            protolink_debug_msg("[SOLAR] FAILURE\r\n");
        }
    }
}
