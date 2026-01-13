// expio_23ioxx.c

#include "expio_23ioxx.h"

// ===============================
// Array global de tarjetas
// ===============================

expio_board_t g_expio_boards[EXPIO_MAX_BOARDS];

// ============================================
// Wrappers a tu driver simple_master / smodbus
// AQUÍ debes adaptar los nombres reales
// ============================================

// Placeholder: ajusta a tu API real
// Debe leer 'qty' registros holding (FC=03) de 'addr' en 'slave_id'
static int1 expio_modbus_read_u16(
   unsigned int8 slave_id,
   unsigned int16 addr,
   unsigned int16 *buf,
   unsigned int8 qty
)
{
   // TODO: reemplaza con tu función real, por ejemplo:
   // return (simple_master_read_holding(slave_id, addr, buf, qty) == 0);
   // Donde 0 indica éxito o ajusta según tu driver.
    return smodbus_read_holding(slave_id,addr,qty,buf);

   // Por ahora dejar como stub para que compile:
   //return 0;
}

// Debe escribir 'qty' registros holding (FC=06 ó 16) comenzando en 'addr'
static int1 expio_modbus_write_u16(
   unsigned int8 slave_id,
   unsigned int16 addr,
   unsigned int16 *buf,
   unsigned int8 qty
)
{
   // TODO: reemplaza con tu función real:
   // return (simple_master_write_multiple(slave_id, addr, buf, qty) == 0);
    return smodbus_write_u16(slave_id,addr,buf);

   //return 0;
}

// ===============================
// Inicialización de una tarjeta
// ===============================

void expio_init_board(
   unsigned int8 idx,
   unsigned int8 slave_id,
   unsigned int8 ch_out,
   unsigned int8 ch_in,
   unsigned int16 base_coil,
   unsigned int16 base_discrete
)
{
   expio_board_t *b;

   if (idx >= EXPIO_MAX_BOARDS)
      return;

   b = &g_expio_boards[idx];

   b->slave_id      = slave_id;
   b->ch_out        = ch_out;
   b->ch_in         = ch_in;
   b->do_state      = 0;
   b->di_state      = 0;
   b->base_coil     = base_coil;
   b->base_discrete = base_discrete;
   b->present       = 1;
}

// ===============================
// Actualizar lectura de DI/DO
// ===============================

int1 expio_update_read(expio_board_t *b)
{
   unsigned int16 reg;

   if (!b->present)
      return 0;

   // Leer DI bits (0x00C0 -> entradas digitales por bit) :contentReference[oaicite:3]{index=3}
   if (!expio_modbus_read_u16(b->slave_id, EXPIO_REG_DI_BITS_BASE, &reg, 1))
      return 0;

   b->di_state = reg;

   // Leer DO bits (0x0070 -> salidas digitales por bit) :contentReference[oaicite:4]{index=4}
   if (!expio_modbus_read_u16(b->slave_id, EXPIO_REG_DO_BITS_BASE, &reg, 1))
      return 0;

   b->do_state = reg;

   return 1;
}

// ===============================
// Escritura de DO hacia la tarjeta
// ===============================

int1 expio_flush_outputs(expio_board_t *b)
{
   unsigned int16 reg;

   if (!b->present)
      return 0;

   reg = b->do_state;

   // Escribimos sólo un registro (primeros 16 canales) en 0x0070 :contentReference[oaicite:5]{index=5}
   if (!expio_modbus_write_u16(b->slave_id, EXPIO_REG_DO_BITS_BASE, &reg, 1))
      return 0;

   return 1;
}

// ===============================
// Set/clear de una salida en el struct
// ===============================

void expio_set_output(expio_board_t *b, unsigned int8 ch, int1 value)
{
   unsigned int16 mask;

   if (!b->present)
      return;

   if (ch == 0 || ch > b->ch_out)
      return;

   mask = ((unsigned int16)1 << (ch - 1));

   if (value)
      b->do_state |= mask;
   else
      b->do_state &= (~mask);
}

// ============================================
// Sincronización de una tarjeta con Protolink
// ============================================
//
// - Lee estado actual de DI/DO desde la tarjeta
// - Actualiza discrete inputs en Protolink
// - Lee coils deseadas en Protolink y si cambian,
//   actualiza b->do_state y escribe a la tarjeta.
//

int1 expio_sync_board(expio_board_t *b)
{
   unsigned int8 i;
   int1 ok;
   int1 dirty;

   if (!b->present)
      return 0;

   ok = expio_update_read(b);
   if (!ok)
      return 0;

   // 1) DI tarjeta -> Protolink discrete
   for (i = 0; i < b->ch_in; i++)
   {
      unsigned int16 mask = ((unsigned int16)1 << i);
      int1 di = ((b->di_state & mask) != 0);

      pl_set_discrete(b->base_discrete + i, di);
   }

   // 2) Coils Protolink -> DO tarjeta
   dirty = 0;
   for (i = 0; i < b->ch_out; i++)
   {
      unsigned int16 mask = ((unsigned int16)1 << i);
      int1 desired = pl_get_coil(b->base_coil + i);
      int1 current = ((b->do_state & mask) != 0);

      if (desired != current)
      {
         // Actualizamos el bit en la máscara local
         if (desired)
            b->do_state |= mask;
         else
            b->do_state &= (~mask);

         dirty = 1;
      }
   }

   // 3) Si hubo cambios, escribir DO a la tarjeta
   if (dirty)
   {
      ok = expio_flush_outputs(b);
      if (!ok)
         return 0;
   }

   return 1;
}

// ===============================
// Task global para todas las tarjetas
// ===============================

void expio_task_all(void)
{
   unsigned int8 i;

   for (i = 0; i < EXPIO_MAX_BOARDS; i++)
   {
      if (g_expio_boards[i].present)
      {
         expio_sync_board(&g_expio_boards[i]);
      }
   }
}

// Sólo lectura de inputs para todas las tarjetas (sin tocar DO)
void expio_poll_all_inputs(void)
{
   unsigned int8 i;

   for (i = 0; i < EXPIO_MAX_BOARDS; i++)
   {
      if (g_expio_boards[i].present)
      {
         expio_update_read(&g_expio_boards[i]);
      }
   }
}
