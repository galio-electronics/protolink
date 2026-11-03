#include "client_rules.h"
static unsigned int8 client_last_inputs;
static int1 client_inputs_valid = FALSE;

void client_rules_init(void)
{
   client_inputs_valid = FALSE;
}

void client_rules_task(unsigned int8 inputs_raw)
{
   if(!client_inputs_valid || inputs_raw != client_last_inputs)
   {
      client_inputs_valid = TRUE;
      client_last_inputs = inputs_raw;
      printf(diagnostics_putc,"EVENT client_inputs=0x%X\r\n",inputs_raw);
   }
   // Implementar reglas solicitadas por el cliente aqui.
   // Se observan entradas crudas; no se activan salidas en esta plantilla.
}
