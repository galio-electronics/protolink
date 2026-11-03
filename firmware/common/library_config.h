#ifndef GALIO_LIBRARY_CONFIG_H
#define GALIO_LIBRARY_CONFIG_H
/* Dependencias disponibles por include path, no inicializadas implicitamente.
 * Habilitar en app_config.h solo despues de revisar docs/dependencies.md.
 * UART1 pertenece a consola; Timer2 pertenece al scheduler.
 */
#ifdef APP_USE_SIMPLE_MASTER
#ifndef SMODBUS_SERIAL_INT
#error Definir SMODBUS_SERIAL_INT explicitamente antes de usar simple_master
#endif
#if (SMODBUS_SERIAL_INT == 7777)
#error UART1 esta reservada para diagnostico
#endif
#include <simple_master/simple_master.c>
#endif

#ifdef APP_USE_RUI3
#ifndef RUI3_AT_STREAM
#error Definir RUI3_AT_STREAM explicitamente antes de usar RUI3
#endif
#if (RUI3_AT_STREAM == 7777)
#error UART1 esta reservada para diagnostico
#endif
// Su .h incluye el .c: no incluir ambos.
#include <rui3_at/rui3_at.h>
#endif

#ifdef APP_USE_OLED
// La aplicacion debe declarar #use i2c(..., STREAM=OLED_I2C) antes.
#include <oled_i2c/oled_ssd1306.h>
#endif

#ifdef APP_USE_LCD
// Definir LCD_I2C_START/WRITE/STOP sobre el stream I2C elegido.
#include <lcd_i2c/lcd_i2c.c>
#endif
#endif
