/* 
 * File:   main.c
 * Author: artga
 *
 * Created on 30 de diciembre de 2025, 05:57 PM
 */

#include "../protolink/excore.h"
#include <bootloader.h>
#include <stdio.h>
#include <stdlib.h>
/*
 * 
 */
void output_leds(boolean state){
    if(state){
        output_high(GPIO_1);
        output_high(GPIO_2);
        output_high(GPIO_3);
        output_high(GPIO_4);
    }else{
        output_low(GPIO_1);
        output_low(GPIO_2);
        output_low(GPIO_3);
        output_low(GPIO_4);
    }
    
}
void main(void){
    protolink_io_init();
    while(TRUE){
        protolink_debug_msg("Hello World\r\n");
        delay_ms(500);
        output_toggle(LED1);     
    }
}

