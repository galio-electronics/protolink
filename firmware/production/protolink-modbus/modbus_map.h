/* 
 * File:   modbus_map.h
 * Author: Arturo Gasca
 *
 * Created on 21 de noviembre de 2025, 10:38 AM
 */

#ifndef MODBUS_MAP_H
#define MODBUS_MAP_H

#include <stdint.h>   // Ajusta si tu compilador CCS usa tipos propios

/* =========================
 *  Tamaños de las áreas Modbus
 * ========================= */
#define MB_NUM_DI        32u     // Discrete Inputs (1x)
#define MB_NUM_COILS     32u     // Coils (0x)
#define MB_NUM_INREG     128u    // Input Registers (3x)
#define MB_NUM_HOLDREG   128u    // Holding Registers (4x)

/* =========================
 *  Mapa lógico (índices)
 *  NOTA: índice 0 => dirección base de cada área
 *        - Coils:          00001 => índice 0
 *        - Discrete Inputs:10001 => índice 0
 *        - Input Registers:30001 => índice 0
 *        - Holding Regs:   40001 => índice 0
 * ========================= */

/* ---- Discrete Inputs (1x) ----
 * 10001?10004 : Entradas digitales
 */
#define MB_DI_DI1        0u      // 10001
#define MB_DI_DI2        1u      // 10002
#define MB_DI_DI3        2u      // 10003
#define MB_DI_DI4        3u      // 10004
// 10005+ reservado para expansión

/* ---- Coils (0x) ----
 * 00001?00004 : Salidas a relevador
 */
#define MB_COIL_DO1      0u      // 00001
#define MB_COIL_DO2      1u      // 00002
#define MB_COIL_DO3      2u      // 00003
#define MB_COIL_DO4      3u      // 00004
// 00005+ reservado para expansión

/* ---- Input Registers (3x) ----
 * 30001?30003 : AI1
 * 30011?30013 : AI2
 * 30021+      : Diagnóstico / Telemetría
 */
#define MB_INREG_AI1_RAW        0u   // 30001 (2 registros para float)
#define MB_INREG_AI1_MA         2u   // 30003 (2 registros para float)
#define MB_INREG_AI1_SCALED     4u   // 30005

#define MB_INREG_AI2_RAW        10u  // 30011 (2 registros para float)
#define MB_INREG_AI2_MA         12u  // 30013 (2 registros para float)
#define MB_INREG_AI2_SCALED     14u  // 30015

#define MB_INREG_BASCULA1       20u  // 30021 (2 registros para float)
#define MB_INREG_BASCULA2       22u  // 30023 (2 registros para float)


/* ---- Holding Registers (4x) ----
 * 40001+ : Configuración general
 */
#define MB_HOLD_FW_VERSION      0u   // 40001: Fw Version (float)
#define MB_HOLD_MODE_OP         2u   // 40003: modo de operación
#define MB_HOLD_SAMPLE_TIME_MS  3u   // 40004: tiempo de muestreo
#define MB_HOLD_AI1_FILTER_EN   4u   // 40005: 0/1
#define MB_HOLD_AI2_FILTER_EN   5u   // 40006: 0/1
#define MB_HOLD_COM_TIMEOUT_S   6u   // 40007: timeout com

#define MB_HOLD_AI1_OFFSET      10u  // 40011: offset AI1 (signed)
#define MB_HOLD_AI1_GAIN_X1000  12u  // 40013: ganancia AI1

#define MB_HOLD_AI2_OFFSET      20u  // 40021
#define MB_HOLD_AI2_GAIN_X1000  22u  // 40023

#define MB_HOLD_DO1_MODE        30u  // 40031: 0=Manual,1=Auto
#define MB_HOLD_DO2_MODE        31u  // 40032
#define MB_HOLD_DO3_MODE        32u  // 40033
#define MB_HOLD_DO4_MODE        33u  // 40034

/* =========================
 *  Estructura general del mapa
 * ========================= */
typedef struct {
   // Bits: usar 0/1 en cada posición
   uint8_t di[MB_NUM_DI];             // 1x Discrete Inputs
   uint8_t coils[MB_NUM_COILS];       // 0x Coils

   // Registros de 16 bits
   uint16_t inreg[MB_NUM_INREG];      // 3x Input Registers
   uint16_t holdreg[MB_NUM_HOLDREG];  // 4x Holding Registers
} mb_map_t;

/* Instancia global (definida en modbus_map.c) */
extern mb_map_t g_mb;

/* API del mapa */
void mb_map_init(void);             // Inicializa valores por defecto
void mb_map_update_inputs(void);    // Lee hardware -> g_mb
void mb_map_apply_outputs(void);    // Aplica g_mb -> hardware (salidas, modos, etc.)

#endif // MODBUS_MAP_H

