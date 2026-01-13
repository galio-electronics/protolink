/* 
 * File:   main.c
 * Author: Arturo Gasca
 *
 * Created on 20 de noviembre de 2025, 01:57 PM
 */
#define PROTOLINK_DEFAULT     1
#include "../protolink/v2.h"
#include <bootloader.h>
#include <stdio.h>
#include <stdlib.h>
#define FW_VERSION "1.0.3"
/*
 * 
 */
void main(void) {
    

    protolink_io_init();
    protolink_timer0_init();
    protolink_interrupts_init();
    protolink_smt_init();
    output_low(LED1);
    output_low(LED2);
    while (true) {
        //output_toggle(LED1);
        //delay_ms(1000);
        //protolink_debug_data("FW Version %s\r\n",FW_VERSION);
        if(protolink_one_second()){                        

            protolink_debug_data("INT0 : %lu pps\r\n",flow1_pps);
            protolink_debug_data("INT1 : %lu pps\r\n",flow2_pps);
            protolink_debug_data("SMT1 : %lu pps\r\n",flow1_smt_pps);
            output_toggle(LED1);
        }
    }

    
}