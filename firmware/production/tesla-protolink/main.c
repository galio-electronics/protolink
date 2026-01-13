/* 
 * Project:   TESLA PROTOLINK
 * Author: Arturo Gasca
 * GALIO ELECTRONICS
 *
 * Created on 30 de diciembre de 2025, 08:11 PM
 */


#include "../../protolink/excore.h"
//#use rs232(baud=1200,parity=N,UART3,bits=8,stream=DISPLAY,errors)
/*Hardware defines*/
#define COMPILACION_DATE    __DATE__
#define COMPILACION_TIME    __TIME__
#define FW_VERSION  "1.0"
#define HW_NAME "Tesla Protolink"
//MODBUS DEFINES
#define MODBUS_PROTOCOL             MODBUS_PROTOCOL_SERIAL
#define MODBUS_TYPE                 MODBUS_TYPE_SLAVE
#define MODBUS_SERIAL_TYPE          MODBUS_RTU     //use MODBUS_ASCII for ASCII mode
#define MODBUS_SERIAL_RX_BUFFER_SIZE 64
#define MODBUS_SERIAL_BAUD          115200
#define MODBUS_PARITY               "NONE"
#define MODBUS_SERIAL_INT_SOURCE    MODBUS_INT_RDA4
#define MODBUS_SERIAL_RX_PIN        RX_MODBUS
#define MODBUS_SERIAL_TX_PIN        TX_MODBUS
#define MODBUS_TIMER_UPDATE         MODBUS_TIMER_ISR
#define MODBUS_TIMER_USED           MODBUS_TIMER_T2
#define MODBUS_SERIAL_ENABLE_PIN    TX_ENABLE
#define MODBUS_SERIAL_RX_ENABLE     RX_ENABLE



#define DEVICE_DEBUG            TRUE
#define DEVICE_SERIAL_SOURCE    UART_INT_RDA2
#define DEVICE_SERIAL_BAUD      9600
#define DEVICE_RST_PIN          PIN_D2 //solo para que compile(no se usa)

#include "devices/generic_uart.h"
#include <bootloader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlibm.h>
#include <ieeefloat.c>
#include <modbus.c>

#define MODBUS_ADDRESS 0x01

// Buffer para recibir la trama (ajustado al tamaño típico)
//char trama[16];
//int trama_index = 0;

int8 coils = 0b00000101;
int8 inputs = 0b00001001;
//int16 hold_regs[] = {0x8800,0x7700,0x6600,0x5500,0x4400,0x3300,0x2200,0x1100};
//int16 input_regs[] = {0x1100,0x2200,0x3300,0x4400,0x5500,0x6600,0x7700,0x8800};
int16 event_count = 0;
int16 input_registers[10];
int16 holding_registers[10];
// Variables extraídas
long long peso_int32 = 0;
long peso_hi, peso_lo;
float peso = 0.0;
char unidad[3] = {0}; // 2 letras + terminador
char estado = 0;
//char trama_to_display[10];



// Convierte un float a la trama de 6 dígitos + signo.
// Formato básico sin punto: "=ABC000S"
// - A,B,C: valor absoluto en kg, invertido (u,d,c)
// - los otros 3 dígitos se quedan en '0'
// - S: '0' si positivo, '-' si negativo
// Si decimals > 0, inserta un '.' en la posición adecuada.
//
// Ejemplos con weight = +/-251 y decimals:
// decimals = 0, +251  -> "=1520000"
// decimals = 0, -251  -> "=152000-"
// decimals = 1, -25.1 -> "=1.52000-"
// decimals = 2, -2.51 -> "=15.2000-"
//
//void build_display_frame(float weight,
//                         unsigned int8 decimals,
//                         char *frame)
//{
//   int1  is_negative;
//   float mag;
//   unsigned int16 scale;
//   unsigned int16 scaled;
//   unsigned int8 c, d, u;
//   char digits[6];
//   unsigned int8 i;
//   unsigned int8 idx = 0;
//   unsigned int8 dec_pos;
//
//   // 1) Signo y magnitud
//   if (weight < 0.0f)
//   {
//      is_negative = TRUE;
//      mag = -weight;
//   }
//   else
//   {
//      is_negative = FALSE;
//      mag = weight;
//   }
//
//   // 2) Escala según decimales (0,1,2...)
//   //    mag * 10^decimals
//   if (decimals == 0)
//      scale = 1;
//   else if (decimals == 1)
//      scale = 10;
//   else if (decimals == 2)
//      scale = 100;
//   else
//   {
//      // Limitar para no crecer de más; 2 decimales es razonable aquí
//      scale = 100;
//      decimals = 2;
//   }
//
//   scaled = (unsigned int16)(mag * (float)scale + 0.5f);
//
//   // 3) Limitar a 3 dígitos (0-999)
//   if (scaled > 99999)
//      scaled = 99999;
//
//   // 4) Separar en centenas, decenas, unidades
//   c = (unsigned int8)(scaled / 100);         // centenas
//   d = (unsigned int8)((scaled / 10) % 10);   // decenas
//   u = (unsigned int8)(scaled % 10);          // unidades
//
//   // 5) Construir los 6 dígitos base: ABC000
//   //    A = unidades, B = decenas, C = centenas
//   digits[0] = '0' + u;
//   digits[1] = '0' + d;
//   digits[2] = '0' + c;
//   digits[3] = '0';
//   digits[4] = '0';
//   digits[5] = '0';
//
//   // 6) Armar la trama final
//   frame[idx++] = '=';
//
//   // Posición del punto decimal (0..5) antes de ese dígito
//   // Ej:
//   //   decimals=0  -> sin punto
//   //   decimals=1  -> "1.52000"
//   //   decimals=2  -> "15.2000"
//   dec_pos = decimals;  // 0: sin punto, 1: antes de digits[1], 2: antes de digits[2], etc.
//
//   for (i = 0; i < 6; i++)
//   {
//      if ((decimals > 0) && (i == dec_pos))
//      {
//         frame[idx++] = '.';   // insertar punto decimal
//      }
//
//      frame[idx++] = digits[i];
//   }
//
//   // 7) Signo al final
//   frame[idx++] = (is_negative ? '-' : '0');
//
//   // 8) Terminador de cadena
//   frame[idx] = '\0';
//}

// Función para procesar la trama completa

void procesar_trama() {
    char peso_str[8]; // Para "+0123.4"
    //Para indicador ricelake 120
    if (data.buffer[0] == 0x02) { // Verifica que inicie con STX
        // Copiar peso
        strncpy(peso_str, &data.buffer[1], 7);
        peso_str[7] = '\0'; // Fin de cadena

        // Copiar unidad
        unidad[0] = data.buffer[8];
        unidad[1] = data.buffer[9];
        unidad[2] = '\0';

        // Copiar estado
        estado = data.buffer[10];

        // Convertir peso
        peso = atof(peso_str);
        output_toggle(GPIO_1);


        // Ahora tienes el peso como float, unidad y estado
        //fprintf(DEBUG,"Peso: %.2f %s\n", peso, unidad);
        //fprintf(DEBUG,"Estado: %c\n", estado);
    }//Para indicador YP200
    else if (data.buffer[0] == 0x3D) {
        strncpy(peso_str, data.buffer + 1, 7);
        peso_str[7] = '\0';
        peso = atof(peso_str);
        output_toggle(GPIO_1);
        //fprintf(DEBUG, "Peso: %.2f \n",peso);
    } else {
        //DEBUG_MSG("WRONG PROTOCOL");
    }
    
}

//int16 value = 0;

int8 swap_bits(int8 c) {
    return ((c & 1) ? 128 : 0) | ((c & 2) ? 64 : 0) | ((c & 4) ? 32 : 0) | ((c & 8) ? 16 : 0) | ((c & 16) ? 8 : 0)
            | ((c & 32) ? 4 : 0) | ((c & 64) ? 2 : 0) | ((c & 128) ? 1 : 0);
}

void procesar_modbus(void) {
    //check address against our address, 0 is broadcast
    if ((modbus_rx.address == MODBUS_ADDRESS) || modbus_rx.address == 0) {
        //output_high(LED1);
        //delay_ms(10);
        output_toggle(LED1);
        switch (modbus_rx.func) {
            case FUNC_READ_COILS: //read coils
                //DEBUG_MSG("[DEBUG] FUNC_READ_COILS \r\n");
                break;
            case FUNC_READ_DISCRETE_INPUT: //read inputs
                //DEBUG_MSG("[DEBUG] FUNC_READ_DISCRETE_INPUT \r\n");
                if (modbus_rx.data[0] || modbus_rx.data[2] ||
                        modbus_rx.data[1] >= 8 || modbus_rx.data[3] + modbus_rx.data[1] > 8)
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_ADDRESS);
                else {
                    int8 data;

                    if (modbus_rx.func == FUNC_READ_COILS)
                        data = coils >> (modbus_rx.data[1]); //move to the starting coil
                    else
                        data = inputs >> (modbus_rx.data[1]); //move to the starting input

                    data = data & (0xFF >> (8 - modbus_rx.data[3])); //0 out values after quantity

                    if (modbus_rx.func == FUNC_READ_COILS)
                        modbus_read_coils_rsp(MODBUS_ADDRESS, 0x01, &data);
                    else
                        modbus_read_discrete_input_rsp(MODBUS_ADDRESS, 0x01, &data);

                    event_count++;
                }
                break;
            case FUNC_READ_HOLDING_REGISTERS:
            case FUNC_READ_INPUT_REGISTERS:

                if (modbus_rx.data[0] || modbus_rx.data[2] ||
                        modbus_rx.data[1] >= 8 || modbus_rx.data[3] + modbus_rx.data[1] > 8) {
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_ADDRESS);
                } else {
                    if (modbus_rx.func == FUNC_READ_HOLDING_REGISTERS) {
                        // DEBUG_MSG("[DEBUG] FUNC_READ_HOLDING_REGISTERS \r\n");
                        modbus_read_holding_registers_rsp(MODBUS_ADDRESS, (modbus_rx.data[3]*2), holding_registers + modbus_rx.data[1]);
                    } else {
                        // DEBUG_MSG("[DEBUG] FUNC_READ_INPUT_REGISTERS \r\n");
                        modbus_read_input_registers_rsp(MODBUS_ADDRESS, (modbus_rx.data[3]*2), input_registers + modbus_rx.data[1]);
                    }

                    event_count++;
                    //output_low(LED1);
                }
                break;
            case FUNC_WRITE_SINGLE_COIL: //write coil
                //DEBUG_MSG("[DEBUG] FUNC_WRITE_SINGLE_COIL \r\n");
                if (modbus_rx.data[0] || modbus_rx.data[3] || modbus_rx.data[1] > 8)
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_ADDRESS);
                else if (modbus_rx.data[2] != 0xFF && modbus_rx.data[2] != 0x00)
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_VALUE);
                else {
                    if (modbus_rx.data[2] == 0xFF)
                        bit_set(coils, modbus_rx.data[1]);
                    else
                        bit_clear(coils, modbus_rx.data[1]);

                    modbus_write_single_coil_rsp(MODBUS_ADDRESS, modbus_rx.data[1], ((int16) (modbus_rx.data[2])) << 8);

                    event_count++;
                }
                break;
            case FUNC_WRITE_SINGLE_REGISTER:
                //DEBUG_MSG("[DEBUG] FUNC_WRITE_SINGLE_REGISTER \r\n");
                if (modbus_rx.data[0] || modbus_rx.data[1] >= 8)
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_ADDRESS);
                else {
                    holding_registers[modbus_rx.data[1]] = make16(modbus_rx.data[2], modbus_rx.data[3]);

                    modbus_write_single_register_rsp(MODBUS_ADDRESS,
                            make16(modbus_rx.data[0], modbus_rx.data[1]),
                            make16(modbus_rx.data[2], modbus_rx.data[3]));
                }
                break;
            case FUNC_WRITE_MULTIPLE_COILS:
                //DEBUG_MSG("[DEBUG] FUNC_WRITE_MULTIPLE_COILS \r\n");
                if (modbus_rx.data[0] || modbus_rx.data[2] ||
                        modbus_rx.data[1] >= 8 || modbus_rx.data[3] + modbus_rx.data[1] > 8)
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_ADDRESS);
                else {
                    int i, j;

                    modbus_rx.data[5] = swap_bits(modbus_rx.data[5]);

                    for (i = modbus_rx.data[1], j = 0; i < modbus_rx.data[1] + modbus_rx.data[3]; ++i, ++j) {
                        if (bit_test(modbus_rx.data[5], j))
                            bit_set(coils, 7 - i);
                        else
                            bit_clear(coils, 7 - i);
                    }

                    modbus_write_multiple_coils_rsp(MODBUS_ADDRESS,
                            make16(modbus_rx.data[0], modbus_rx.data[1]),
                            make16(modbus_rx.data[2], modbus_rx.data[3]));

                    event_count++;
                }
                break;
            case FUNC_WRITE_MULTIPLE_REGISTERS:
                // DEBUG_MSG("[DEBUG] FUNC_WRITE_MULTIPLE_REGISTERS \r\n");
                if (modbus_rx.data[0] || modbus_rx.data[2] ||
                        modbus_rx.data[1] >= 8 || modbus_rx.data[3] + modbus_rx.data[1] > 8)
                    modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_DATA_ADDRESS);
                else {
                    int i, j;

                    for (i = 0, j = 5; i < modbus_rx.data[4] / 2; ++i, j += 2)
                        holding_registers[i] = make16(modbus_rx.data[j], modbus_rx.data[j + 1]);

                    modbus_write_multiple_registers_rsp(MODBUS_ADDRESS,
                            make16(modbus_rx.data[0], modbus_rx.data[1]),
                            make16(modbus_rx.data[2], modbus_rx.data[3]));

                    event_count++;
                }
                break;
            default: //We don't support the function, so return exception
                //DEBUG_MSG("[DEBUG] We don´t Support the function \r\n");
                modbus_exception_rsp(MODBUS_ADDRESS, modbus_rx.func, ILLEGAL_FUNCTION);
        }
    }

}


void main(void){
    protolink_io_init();
     int i;
    for (i = 0; i < 10; i++) {
        input_registers[i] = 0x0000;
        holding_registers[i] = 0x0000;

    }
     
     modbus_init();
    device_init();
    enable_interrupts(GLOBAL);
    //DEBUG_MSG("Reset!\r\n");
    fprintf(DEBUG,"name=%s,version=%s,date=%s,time=%s\n",HW_NAME,FW_VERSION,COMPILACION_DATE,COMPILACION_TIME);
    output_low(LED1);
    output_low(GPIO_1);
    delay_ms(500);
    while(TRUE){
        //protolink_debug_msg("Hello World\r\n");
        if (data.AVAILABLE) {            
            data.AVAILABLE = FALSE;
            procesar_trama();
            peso_int32 = f_PICtoIEEE(peso);
            peso_hi = (int16) (peso_int32 >> 16); // Parte alta (bits 31?16)
            peso_lo = (int16) (peso_int32 & 0xFFFF); // Parte baja (bits 15?0)
            input_registers[0] = peso_hi;
            input_registers[1] = peso_lo;
            
            //build_display_frame(peso, 0, trama_to_display);
            //fprintf(DISPLAY,"%s",trama_to_display);
        }
        if (modbus_kbhit()) {
            procesar_modbus();

        }
    }
}

