/* Plantilla cliente: un main y librerias incluidas una sola vez. */
#include "app_config.h"
#include "../../../station.h"
#include <string.h>
#include "../../common/diagnostics.h"
#include "../../common/library_config.h"
#include "client_rules.c"

void app_init(void) { client_rules_init(); }
void app_task_10ms(unsigned int8 inputs_raw) { client_rules_task(inputs_raw); }
void app_task_100ms(void) { }
void app_task_1000ms(void) { }

#include "../../common/station_app.c"
