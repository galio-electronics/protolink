/* Fuente principal del firmware maestro. Editar este perfil y sus modulos. */
#include "app_config.h"
#include "../../station.h"
#include <string.h>
#include "../common/diagnostics.h"
#include "../common/library_config.h"
#include "../common/event_station.c"

void app_init(void)
{
   // Inicializacion especifica del producto, despues de GPIO/diagnostico.
}

void app_task_10ms(unsigned int8 inputs_raw)
{
   // Agregar funciones del firmware maestro aqui. No usar delay_ms().
}

void app_task_100ms(void)
{
}

void app_task_1000ms(void)
{
}

#include "../common/station_app.c"
