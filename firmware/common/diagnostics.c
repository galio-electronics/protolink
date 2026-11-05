#include "diagnostics.h"
#if (STATION_DEBUG_ENABLED != 1) || (STATION_DEBUG_UART != 1)
#error La consola comun requiere diagnostico en UART1
#endif
volatile char diagnostics_tx[128];
volatile unsigned int8 diagnostics_tx_head = 0;
volatile unsigned int8 diagnostics_tx_tail = 0;
volatile char diagnostics_rx[64];
volatile unsigned int8 diagnostics_rx_head = 0;
volatile unsigned int8 diagnostics_rx_tail = 0;
volatile unsigned int16 diagnostics_rx_dropped = 0;
unsigned int16 diagnostics_tx_dropped = 0;
unsigned int16 diagnostics_reset_cause;

#INT_TBE
void diagnostics_tx_isr(void)
{
   if(diagnostics_tx_head != diagnostics_tx_tail)
   {
      fputc(diagnostics_tx[diagnostics_tx_tail],DEBUG);
      diagnostics_tx_tail = (diagnostics_tx_tail + 1) & 127;
   }
   else
      disable_interrupts(INT_TBE);
}

#INT_RDA
void diagnostics_rx_isr(void)
{
   char value = fgetc(DEBUG);
   unsigned int8 next = (diagnostics_rx_head + 1) & 63;
   if(next == diagnostics_rx_tail)
      ++diagnostics_rx_dropped;
   else
   {
      diagnostics_rx[diagnostics_rx_head] = value;
      diagnostics_rx_head = next;
   }
}

// Callback CCS para printf; unicamente desde main. Nunca espera al UART.
void diagnostics_putc(char value)
{
   unsigned int8 next = (diagnostics_tx_head + 1) & 127;
   if(next == diagnostics_tx_tail)
   {
      ++diagnostics_tx_dropped;
      return;
   }
   diagnostics_tx[diagnostics_tx_head] = value;
   diagnostics_tx_head = next;
   enable_interrupts(INT_TBE);
}

void diagnostics_init(void)
{
   disable_interrupts(INT_TBE);
   disable_interrupts(INT_RDA);
   diagnostics_tx_head = diagnostics_tx_tail = 0;
   diagnostics_rx_head = diagnostics_rx_tail = 0;
   diagnostics_tx_dropped = diagnostics_rx_dropped = 0;
   diagnostics_reset_cause = restart_cause();
   enable_interrupts(INT_RDA);
}

void diagnostics_info(void)
{
   printf(diagnostics_putc,"INFO app=%s fw=%s build=%s %s reset=0x%LX\r\n",
          APP_NAME,FW_VERSION,__DATE__,__TIME__,diagnostics_reset_cause);
}

void diagnostics_status(unsigned int8 inputs_raw)
{
   unsigned int16 rx_dropped;
   disable_interrupts(INT_RDA);
   rx_dropped = diagnostics_rx_dropped;
   enable_interrupts(INT_RDA);
   printf(diagnostics_putc,"STAT t=%LU di_raw=0x%X tx_drop=%LU rx_drop=%LU\r\n",
          protolink_millis(),inputs_raw,(unsigned int32)diagnostics_tx_dropped,
          (unsigned int32)rx_dropped);
}

#ifdef APP_EVENT_MANAGER
#include "event_diagnostics.c"
#endif
void diagnostics_task(unsigned int8 inputs_raw)
{
   static char command[24];
   static unsigned int8 length = 0;
   static int1 overflow = FALSE;
   char value;
   // Un caracter por vuelta: trabajo acotado.
   if(diagnostics_rx_tail == diagnostics_rx_head) return;
   value = diagnostics_rx[diagnostics_rx_tail];
   diagnostics_rx_tail = (diagnostics_rx_tail + 1) & 63;
   if(value == '\r' || value == '\n')
   {
      if(overflow)
         printf(diagnostics_putc,"ERR command_too_long\r\n");
      else if(length)
      {
         command[length] = 0;
#ifdef APP_EVENT_MANAGER
         if(event_diagnostics_command(command)) { }
         else
#endif
         if(!strcmp(command,"info")) diagnostics_info();
         else if(!strcmp(command,"status") || !strcmp(command,"inputs") || !strcmp(command,"errors"))
            diagnostics_status(inputs_raw);
#ifdef APP_DIAGNOSTICS_COMMAND
         else if(APP_DIAGNOSTICS_COMMAND(command)) { }
#endif
         else
            printf(diagnostics_putc,"HELP info status inputs errors\r\n");
      }
      length = 0;
      overflow = FALSE;
   }
   else if(length < sizeof(command)-1 && !overflow)
      command[length++] = value;
   else
      overflow = TRUE;
}
