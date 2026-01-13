// expio_23ioxx.h
// Capa de aplicación para expansoras 23IOXXX (ej. 23IOB16)
// Pensado para CCS C + driver simple_master/smodbus

#ifndef _EXPIO_23IOXX_H_
#define _EXPIO_23IOXX_H_

// =========================
// Parámetros del sistema
// =========================

#define EXPIO_MAX_BOARDS      4   // Ajusta según tus necesidades
#define EXPIO_MAX_CHANNELS    16  // Para 23IOB16: 16 IN / 16 OUT

// =========================
// Mapa de registros 23IOXXX
// (según manual)
// =========================

// Salidas por bit (una máscara por 16 canales)
#define EXPIO_REG_DO_BITS_BASE    0x0070  // 0x0070-0x0072 -> DO0-DO47 :contentReference[oaicite:1]{index=1}

// Entradas por bit (una máscara por 16 canales)
#define EXPIO_REG_DI_BITS_BASE    0x00C0  // 0x00C0-0x00C2 -> DI0-DI47 :contentReference[oaicite:2]{index=2}

// Nota: Para una tarjeta de 16 canales, sólo usamos el primer registro
// 0x0070 para DO y 0x00C0 para DI (bits 0-15).

// =====================================
// Hooks a tu mapa Modbus de Protolink
// (DEBES implementarlos en tu firmware)
// =====================================

// Lee/escribe coils (salidas lógicas) del mapa Modbus de Protolink
extern int1 pl_get_coil(unsigned int16 index);
extern void pl_set_coil(unsigned int16 index, int1 value);

// Lee/escribe discrete inputs (entradas lógicas) del mapa Modbus de Protolink
extern int1 pl_get_discrete(unsigned int16 index);
extern void pl_set_discrete(unsigned int16 index, int1 value);

// =====================================
// Estructura de una tarjeta de expansión
// =====================================

typedef struct
{
   unsigned int8  slave_id;       // Dirección Modbus RTU de la tarjeta
   unsigned int8  ch_out;         // Número de salidas (típicamente 16)
   unsigned int8  ch_in;          // Número de entradas (típicamente 16)

   unsigned int16 do_state;       // Bitmask de salidas en la tarjeta
   unsigned int16 di_state;       // Bitmask de entradas en la tarjeta

   unsigned int16 base_coil;      // Índice base en el mapa de coils de Protolink
   unsigned int16 base_discrete;  // Índice base en el mapa de discrete inputs

   int1           present;        // Para habilitar/deshabilitar tarjeta
} expio_board_t;

// Array global de tarjetas (declarado en el .c)
extern expio_board_t g_expio_boards[EXPIO_MAX_BOARDS];

// =========================
// API pública
// =========================

// Inicializa una tarjeta concreta
void expio_init_board(
   unsigned int8 idx,
   unsigned int8 slave_id,
   unsigned int8 ch_out,
   unsigned int8 ch_in,
   unsigned int16 base_coil,
   unsigned int16 base_discrete
);

// Lee DI/DO desde la tarjeta y actualiza su estructura
int1 expio_update_read(expio_board_t *b);

// Aplica cambios de DO desde la estructura hacia la tarjeta (escritura Modbus)
int1 expio_flush_outputs(expio_board_t *b);

// Set/clear de una salida en la estructura (NO escribe aún en Modbus)
void expio_set_output(expio_board_t *b, unsigned int8 ch, int1 value);

// Sincroniza una tarjeta con el mapa Protolink:
// - DI: tarjeta -> Protolink discrete
// - DO: Protolink coils -> tarjeta -> Modbus
int1 expio_sync_board(expio_board_t *b);

// Task para todas las tarjetas (llamar periódicamente en tu main)
void expio_task_all(void);

// Opcional: fuerza lectura completa de todas las tarjetas (sin escribir DO)
void expio_poll_all_inputs(void);

#endif  // _EXPIO_23IOXX_H_
