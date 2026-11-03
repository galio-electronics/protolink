#ifndef GALIO_DIAGNOSTICS_H
#define GALIO_DIAGNOSTICS_H
// Solo UART1. Buffer TX 128 bytes, RX 64 bytes. Sin printf desde ISR.
void diagnostics_putc(char value);
void diagnostics_init(void);
void diagnostics_info(void);
void diagnostics_status(unsigned int8 inputs_raw);
void diagnostics_task(unsigned int8 inputs_raw);
#endif
