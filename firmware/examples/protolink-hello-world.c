/* 
 * File:   main.c
 * Author: Arturo Gasca
 *
 * Created on 20 de noviembre de 2025, 01:57 PM
 */
#define PROTOLINK_DEFAULT true //comment this line for unable default functions
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
    output_low(LED1);
    output_low(LED2);
    while (true) {
        if(protolink_one_second()){
            output_toggle(LED1);
            protolink_debug_data("FW Version %s\r\n",FW_VERSION);   
        }
    }
}