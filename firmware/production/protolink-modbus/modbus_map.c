/* 
 * File:   modbus_map.c
 * Author: Arturo Gasca
 *
 * Created on 21 de noviembre de 2025, 10:39 AM
 */
#include "../../protolink/v2.h"
#include "modbus_map.h"

#include "../../../libs/analog_drivers/ads1115.c"
#include "../../../libs/analog_drivers/ina281.c"
#include "../../../libs/uart_receiver/generic_uart.h"



// Variables para display RS232

// Variables extraídas
long long peso_int32 = 0;
long peso_hi, peso_lo;
float peso = 0.0;
char unidad[3] = {0}; // 2 letras + terminador
char estado = 0;

// Valores reales (lo que "deberia" ser)
float I_real_low  = 4.000f;
float I_real_high = 20.000f;

// Valores medidos por tu driver actual
// (los que obtuviste en los pasos anteriores)
float I_meas_low = 3.980f;   // adc_read_avg_mA en 4 mA
float I_meas_high = 19.950f;  // adc_read_avg_mA en 20 mA

float loop0_gain_cal   = (I_real_high - I_real_low) / (I_meas_high - I_meas_low);
float loop0_offset_cal = I_real_low - (loop0_gain_cal * I_meas_low);

float loop1_gain_cal   = 1.0f;
float loop1_offset_cal = 0.0f;


ina281_ch_t loop_ch0;
ina281_ch_t loop_ch1;

// Define aquí tus funciones / macros de hardware reales
// (ajusta nombres según tu proyecto)
#if 1
   #define READ_DI1()         input(IN_0)   // ejemplo
   #define READ_DI2()         input(IN_1)
   #define READ_DI3()         input(IN_2)
   #define READ_DI4()         input(IN_3)
#endif
#if 0
   uint16_t read_ai1_raw(void);
   uint16_t read_ai2_raw(void);
   int16_t  get_temp_internal_c_x10(void);
   uint16_t get_vbus_mV(void);
   uint16_t get_ibus_mA(void);
#endif
   
// Funciones de lectura para loop CH0 y  CH1
#if 1
   float loop0_read_mA(void);
   float loop1_read_mA(void);
#endif


mb_map_t g_mb;

/* =========================
 *  Inicialización
 * ========================= */
void mb_map_init(void)
{
    uint16_t i;

    // Limpia todo
    for(i = 0; i < MB_NUM_DI; i++)      g_mb.di[i] = 0;
    for(i = 0; i < MB_NUM_COILS; i++)   g_mb.coils[i] = 0;
    for(i = 0; i < MB_NUM_INREG; i++)   g_mb.inreg[i] = 0;
    for(i = 0; i < MB_NUM_HOLDREG; i++) g_mb.holdreg[i] = 0;

    // Holding registers por defecto (config)
    g_mb.holdreg[MB_HOLD_MODE_OP]        = 0;      // Normal
    g_mb.holdreg[MB_HOLD_SAMPLE_TIME_MS] = 1000;   // 1 s
    g_mb.holdreg[MB_HOLD_AI1_FILTER_EN]  = 1;
    g_mb.holdreg[MB_HOLD_AI2_FILTER_EN]  = 1;
    g_mb.holdreg[MB_HOLD_COM_TIMEOUT_S]  = 30;

    g_mb.holdreg[MB_HOLD_AI1_OFFSET]     = 0;
    g_mb.holdreg[MB_HOLD_AI1_GAIN_X1000] = 1000;   // 1.000

    g_mb.holdreg[MB_HOLD_AI2_OFFSET]     = 0;
    g_mb.holdreg[MB_HOLD_AI2_GAIN_X1000] = 1000;

    g_mb.holdreg[MB_HOLD_DO1_MODE]       = 0;      // Manual
    g_mb.holdreg[MB_HOLD_DO2_MODE]       = 0;
    g_mb.holdreg[MB_HOLD_DO3_MODE]       = 0;
    g_mb.holdreg[MB_HOLD_DO4_MODE]       = 0;
    
    device_init(); //incializa para recibir datos de rs232 para display
    ads1115_init();
    
     // Canal 0 y 1 del ADS1115 conectado al INA281A1 + Rshunt 12.4 ohms
    ina281_init_ch(&loop_ch0,
                    0,         // ads_channel
                    20.0f,     // gain INA281A1 = 20 V/V
                    12.4f,     // Rshunt = 12.4 ohms
                    0.0f);     // offset_volts (lo calibramos despues)
    ina281_init_ch(&loop_ch1,
                    1,         // ads_channel
                    20.0f,     // gain INA281A1 = 20 V/V
                    12.4f,     // Rshunt = 12.4 ohms
                    0.0f);     // offset_volts (lo calibramos despues)
}

/* =========================
 *  Actualizar entradas
 *  HW -> mapa Modbus
 * ========================= */
void mb_map_update_inputs(void)
{
   // ---- Entradas digitales ----
   // TODO: remplaza READ_DIx() por tus funciones reales
#if 1
    g_mb.di[MB_DI_DI1] = READ_DI1() ? 1u : 0u;
    g_mb.di[MB_DI_DI2] = READ_DI2() ? 1u : 0u;
    g_mb.di[MB_DI_DI3] = READ_DI3() ? 1u : 0u;
    g_mb.di[MB_DI_DI4] = READ_DI4() ? 1u : 0u;
#endif

   // ---- Entradas analógicas ----
    
#if 1
    long long ch0_420_int32;
    long long ch1_420_int32;
    float i_mA_ch0 = loop0_read_mA();
    float i_mA_ch1 = loop1_read_mA();
    //protolink_debug_data("CH0: %.2f\r\n",i_mA_ch0);
    //protolink_debug_data("CH1: %.2f\r\n",i_mA_ch1);
    //delay_ms(500);
    
    ch0_420_int32 = f_PICtoIEEE(i_mA_ch0);
    ch1_420_int32 = f_PICtoIEEE(i_mA_ch1);
    g_mb.inreg[MB_INREG_AI1_MA] =(int16) (ch0_420_int32 >> 16);
    g_mb.inreg[MB_INREG_AI1_MA + 1] = (int16) (ch0_420_int32 & 0xFFFF) ;
    
    g_mb.inreg[MB_INREG_AI2_MA] =(int16) (ch1_420_int32 >> 16);
    g_mb.inreg[MB_INREG_AI2_MA + 1] = (int16) (ch1_420_int32 & 0xFFFF) ;   
    
    
//    fprintf(DEBUG,"I_mA_ch0=%.3f, IEEE=0x%08LX\r\n",
//                     i_mA_ch0, ch0_420_int32);
    
    /*protolink_debug_data("Reg %d ",MB_INREG_AI1_MA);
    protolink_debug_data("value %ld \r\n",g_mb.inreg[MB_INREG_AI1_MA]);
    protolink_debug_data("Reg %d ",MB_INREG_AI1_MA+1);
    protolink_debug_data("value %ld \r\n",g_mb.inreg[MB_INREG_AI1_MA+1]);
    protolink_debug_data("Reg %d ",MB_INREG_AI2_MA);
    protolink_debug_data("value %ld \r\n",g_mb.inreg[MB_INREG_AI2_MA]);
    protolink_debug_data("Reg %d ",MB_INREG_AI2_MA +1);
    protolink_debug_data("value %ld \r\n",g_mb.inreg[MB_INREG_AI2_MA+1]);*/
#endif
#if 0
   uint16_t raw1 = read_ai1_raw();
   uint16_t raw2 = read_ai2_raw();

   // Ejemplo de cálculo en mA (ajusta escala real)
   // Supongamos ADC 0?4095 = 0?20 mA
   uint16_t i1_mA_x100 = (uint32_t)raw1 * 2000u / 4095u;  // mA *100
   uint16_t i2_mA_x100 = (uint32_t)raw2 * 2000u / 4095u;

   // Aplica calibración de holding registers
   int16_t offset1 = (int16_t)g_mb.holdreg[MB_HOLD_AI1_OFFSET];
   int16_t offset2 = (int16_t)g_mb.holdreg[MB_HOLD_AI2_OFFSET];
   uint16_t gain1  = g_mb.holdreg[MB_HOLD_AI1_GAIN_X1000];
   uint16_t gain2  = g_mb.holdreg[MB_HOLD_AI2_GAIN_X1000];

   int32_t temp1 = ((int32_t)i1_mA_x100 * gain1) / 1000 + offset1;
   int32_t temp2 = ((int32_t)i2_mA_x100 * gain2) / 1000 + offset2;

   if(temp1 < 0) temp1 = 0;
   if(temp2 < 0) temp2 = 0;

   // AI1
   g_mb.inreg[MB_INREG_AI1_RAW]     = raw1;
   g_mb.inreg[MB_INREG_AI1_MA_X100] = (uint16_t)i1_mA_x100;
   g_mb.inreg[MB_INREG_AI1_SCALED]  = (uint16_t)temp1;   // ej. 0?1000

   // AI2
   g_mb.inreg[MB_INREG_AI2_RAW]     = raw2;
   g_mb.inreg[MB_INREG_AI2_MA_X100] = (uint16_t)i2_mA_x100;
   g_mb.inreg[MB_INREG_AI2_SCALED]  = (uint16_t)temp2;
#endif

   // ---- Telemetría / diagnóstico ----
#if 0
   int16_t temp_x10 = get_temp_internal_c_x10();
   g_mb.inreg[MB_INREG_TEMP_INT] = (uint16_t)temp_x10;

   g_mb.inreg[MB_INREG_VBUS_MV]  = get_vbus_mV();
   g_mb.inreg[MB_INREG_IBUS_MA]  = get_ibus_mA();
#endif
}

/* =========================
 *  Aplicar salidas
 *  mapa Modbus -> HW
 * ========================= */
void mb_map_apply_outputs(void)
{
   // Salidas DO controladas por coils
    output_bit(OUT_0, g_mb.coils[MB_COIL_DO1] ? 1 : 0);
    output_bit(OUT_1, g_mb.coils[MB_COIL_DO2] ? 1 : 0);
    output_bit(OUT_2, g_mb.coils[MB_COIL_DO3] ? 1 : 0);
    output_bit(OUT_3, g_mb.coils[MB_COIL_DO4] ? 1 : 0);


   // Aquí podrías aplicar lógica según MB_HOLD_DOx_MODE
   // (manual/auto) en base a sensores, pero eso ya es
   // business logic de tu equipo.
}


float loop0_read_mA(void)
{
    float I_meas = ina281_read_current_mA(&loop_ch0);  // sin calibración
    return (loop0_gain_cal * I_meas) + loop0_offset_cal;
}

float loop1_read_mA(void)
{
    float I_meas = ina281_read_current_mA(&loop_ch1);  // sin calibración
    return (loop1_gain_cal * I_meas) + loop1_offset_cal;
}

 //Función para procesar la trama completa

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
        //output_toggle(LED2);
        //protolink_debug_data("[DEBUG]Display Peso: %.2f\r\n",peso);


        // Ahora tienes el peso como float, unidad y estado
        //fprintf(DEBUG,"Peso: %.2f %s\n", peso, unidad);
        //fprintf(DEBUG,"Estado: %c\n", estado);
    }//Para indicador YP200
    else if (data.buffer[0] == 0x3D) {
        strncpy(peso_str, data.buffer + 1, 7);
        peso_str[7] = '\0';
        peso = atof(peso_str);
        //output_toggle(LED2);
        //protolink_debug_data("[DEBUG]Display Peso: %.2f\r\n",peso);
        //fprintf(DEBUG, "Peso: %.2f \n",peso);
    } else {
        //DEBUG_MSG("WRONG PROTOCOL");
    }
    
}




void update_rs232_display(void)
{
    if (data.AVAILABLE) {
        data.AVAILABLE = FALSE;
        procesar_trama();
        peso_int32 = f_PICtoIEEE(peso);
        peso_hi = (int16) (peso_int32 >> 16); // Parte alta (bits 31?16)
        peso_lo = (int16) (peso_int32 & 0xFFFF); // Parte baja (bits 15?0)
        g_mb.inreg[MB_INREG_BASCULA1] = peso_hi;
        g_mb.inreg[MB_INREG_BASCULA1 +1 ] = peso_lo;       
    }

    
}