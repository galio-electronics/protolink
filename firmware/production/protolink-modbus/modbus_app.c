#include "../../protolink/v2.h"
#include "modbus_map.h"
#include "../../../libs/modbus/modbus.c" // Ajusta segun estructura de libs


// Helpers CCS
//#define MAKE16(h,l)  (((uint16_t)(h) << 8) | (uint16_t)(l))

static void mb_handle_read_coils(void);
static void mb_handle_read_discrete_inputs(void);
static void mb_handle_read_input_registers(void);
static void mb_handle_read_holding_registers(void);
static void mb_handle_write_single_coil(void);
static void mb_handle_write_multiple_coils(void);
static void mb_handle_write_single_register(void);
static void mb_handle_write_multiple_registers(void);

/* =========================
 *  Llamar en el main loop
 * ========================= */
void mb_app_task(void)
{
   update_rs232_display();
    // Primero actualiza el mapa desde hardware
   mb_map_update_inputs();
   
   if (!modbus_kbhit()) 
       return;
   
   if(modbus_rx.error){
       //TODO: Log errors
       return;
   }
   switch (modbus_rx.func){
       case FUNC_READ_COILS:
            mb_handle_read_coils();
            protolink_debug_msg("[DEBUG ]FUNC_READ_COILS\r\n");
            output_toggle(LED2);
            break;

        case FUNC_READ_DISCRETE_INPUT:
           mb_handle_read_discrete_inputs();
           protolink_debug_msg("[DEBUG ]FUNC_READ_DISCRETE_INPUT\r\n");
           output_toggle(LED2);
           break;

        case FUNC_READ_INPUT_REGISTERS:
           mb_handle_read_input_registers();
           protolink_debug_msg("[DEBUG ]FUNC_READ_INPUT_REGISTERS\r\n");
           output_toggle(LED2);
           break;

        case FUNC_READ_HOLDING_REGISTERS:
           mb_handle_read_holding_registers();
           protolink_debug_msg("[DEBUG ]FUNC_READ_HOLDING_REGISTERS\r\n");
           output_toggle(LED2);
           break;

        case FUNC_WRITE_SINGLE_COIL:
           mb_handle_write_single_coil();
           protolink_debug_msg("[DEBUG ]FUNC_WRITE_SINGLE_COIL\r\n");
           output_toggle(LED2);
           break;

        case FUNC_WRITE_MULTIPLE_COILS:
           mb_handle_write_multiple_coils();
           protolink_debug_msg("[DEBUG ]FUNC_WRITE_MULTIPLE_COILS\r\n");
           output_toggle(LED2);
           break;

        case FUNC_WRITE_SINGLE_REGISTER:
           mb_handle_write_single_register();
           protolink_debug_msg("[DEBUG ]FUNC_WRITE_SINGLE_REGISTER\r\n");
           output_toggle(LED2);
           break;

        case FUNC_WRITE_MULTIPLE_REGISTERS:
           mb_handle_write_multiple_registers();
           protolink_debug_msg("[DEBUG ]FUNC_WRITE_MULTIPLE_REGISTERS\r\n");
           output_toggle(LED2);
           break;

        default:
            protolink_debug_msg("[DEBUG ]DEFAULT EXCEPCION\r\n");
            output_high(LED2);
           // TODO: si tu modbus.c tiene respuesta de excepción,
           // envía excepción ILLEGAL_FUNCTION aquí.
           break;
       
   }
   // Aplica salidas según mapa (coils / configs)
   mb_map_apply_outputs();
}

/* =========================
 *  READ COILS (0x)
 * ========================= */
static void mb_handle_read_coils(void)
{
   uint16_t start = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t qty   = make16(modbus_rx.data[2], modbus_rx.data[3]);

   // start y qty se interpretan como offset 0-base
   if((start + qty) > MB_NUM_COILS)
   {
      // TODO: responder excepción ILLEGAL_DATA_ADDRESS
      return;
   }

   // Empaquetar bits en bytes para la respuesta
   // Buffer temporal (ajusta tamaño según qty)
   uint8_t buf[MB_NUM_COILS/8 + 1];
   uint16_t i;
   uint8_t bitMask = 1;
   uint8_t byteIdx = 0;

   for(i = 0; i < qty; i++)
   {
      if(i % 8 == 0)
      {
         buf[byteIdx] = 0;
         bitMask = 1;
      }

      if(g_mb.coils[start + i])
         buf[byteIdx] |= bitMask;

      bitMask <<= 1;

      if((i % 8) == 7)
         byteIdx++;
   }
   if(qty % 8)
      byteIdx++;

   // TODO: usa la función de tu modbus.c para responder
   // Ejemplo orientativo:
   // modbus_read_coils_rsp(byteIdx, buf);
    modbus_read_coils_rsp(MODBUS_ADDRESS,byteIdx,&buf);
}

/* =========================
 *  READ DISCRETE INPUTS (1x)
 * ========================= */
static void mb_handle_read_discrete_inputs(void)
{

    uint16_t start = make16(modbus_rx.data[0], modbus_rx.data[1]);
    uint16_t qty   = make16(modbus_rx.data[2], modbus_rx.data[3]);
   if((start + qty) > MB_NUM_DI)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }

   uint8_t buf[MB_NUM_DI/8 + 1];
   uint16_t i;
   uint8_t bitMask = 1;
   uint8_t byteIdx = 0;

   for(i = 0; i < qty; i++)
   {
      if(i % 8 == 0)
      {
         buf[byteIdx] = 0;
         bitMask = 1;
      }

      if(g_mb.di[start + i])
         buf[byteIdx] |= bitMask;

      bitMask <<= 1;

      if((i % 8) == 7)
         byteIdx++;
   }
   if(qty % 8)
      byteIdx++;
   
   modbus_read_discrete_input_rsp(MODBUS_ADDRESS,byteIdx,&buf);

   // TODO: función respuesta discrete inputs
   // modbus_read_discrete_inputs_rsp(byteIdx, buf);
}

/* =========================
 *  READ INPUT REGISTERS (3x)
 * ========================= */
static void mb_handle_read_input_registers(void)
{
   uint16_t start = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t qty   = make16(modbus_rx.data[2], modbus_rx.data[3]);
   if((start + qty) > MB_NUM_INREG)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }
   modbus_read_input_registers_rsp(MODBUS_ADDRESS,qty*2,&g_mb.inreg[start]);
}

/* =========================
 *  READ HOLDING REGISTERS (4x)
 * ========================= */
static void mb_handle_read_holding_registers(void)
{
   uint16_t start = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t qty   = make16(modbus_rx.data[2], modbus_rx.data[3]);
   uint16_t i;

   if((start + qty) > MB_NUM_HOLDREG)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }
    modbus_read_holding_registers_rsp(MODBUS_ADDRESS,qty*2,&g_mb.holdreg[start]);
}

/* =========================
 *  WRITE SINGLE COIL
 * ========================= */
static void mb_handle_write_single_coil(void)
{
   uint16_t addr = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t val  = make16(modbus_rx.data[2], modbus_rx.data[3]);
   
   protolink_debug_data("Addr: %ld\r\n",addr);
   protolink_debug_data("Val: %ld\r\n",val);

   if(addr >= MB_NUM_COILS)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }

   g_mb.coils[addr] = (val == 0xFF00u) ? 1u : 0u;

   // Respuesta eco (Modbus estándar)
   // TODO: función correspondiente en tu modbus.c
   // modbus_write_single_coil_rsp(addr, val);
   
   modbus_write_single_coil_rsp(MODBUS_ADDRESS,addr,val);
           
}

/* =========================
 *  WRITE MULTIPLE COILS
 * ========================= */
static void mb_handle_write_multiple_coils(void)
{
   uint16_t start = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t qty   = make16(modbus_rx.data[2], modbus_rx.data[3]);
   //uint8_t  byteCount = modbus_rx.data[4];

   if((start + qty) > MB_NUM_COILS)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }

   uint8_t *p = &modbus_rx.data[5];
   uint16_t i;
   uint8_t bitMask = 1;
   uint8_t currentByte = *p;
   uint8_t bitIndex = 0;

   for(i = 0; i < qty; i++)
   {
      if(bitIndex == 8)
      {
         p++;
         currentByte = *p;
         bitIndex = 0;
         bitMask = 1;
      }

      g_mb.coils[start + i] = (currentByte & bitMask) ? 1u : 0u;

      bitMask <<= 1;
      bitIndex++;
   }

   // Respuesta eco de start y qty
   modbus_write_multiple_coils_rsp(MODBUS_ADDRESS,start, qty);
}

/* =========================
 *  WRITE SINGLE REGISTER
 * ========================= */
static void mb_handle_write_single_register(void)
{
   uint16_t addr = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t val  = make16(modbus_rx.data[2], modbus_rx.data[3]);

   if(addr >= MB_NUM_HOLDREG)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }

   g_mb.holdreg[addr] = val;

   // Respuesta eco
   modbus_write_single_register_rsp(MODBUS_ADDRESS,addr, val);
}

/* =========================
 *  WRITE MULTIPLE REGISTERS
 * ========================= */
static void mb_handle_write_multiple_registers(void)
{
   uint16_t start = make16(modbus_rx.data[0], modbus_rx.data[1]);
   uint16_t qty   = make16(modbus_rx.data[2], modbus_rx.data[3]);
   //uint8_t  byteCount = modbus_rx.data[4];

   if((start + qty) > MB_NUM_HOLDREG)
   {
      // TODO: excepción
       modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_DATA_ADDRESS);
      return;
   }

   uint8_t *p = &modbus_rx.data[5];
   uint16_t i;

   for(i = 0; i < qty; i++)
   {
      uint16_t v = make16(p[0], p[1]);
      g_mb.holdreg[start + i] = v;
      p += 2;
   }

   // Respuesta eco de start y qty
    modbus_write_multiple_registers_rsp(MODBUS_ADDRESS,start, qty);
}
