/* Uppercase commands are distinct from existing demo 'log ...' commands. */
static ev_bool event_diag_reading=0;
static ev_u8 event_diag_health=255,event_diag_detail=0;
static ev_bool event_diag_started=0,event_diag_scan_reported=0;
static ev_u8 event_diag_last_state=255;
static void event_diag_state(ev_u8 state)
{
   switch(state) {
      case EH_UNKNOWN: printf(diagnostics_putc,"SIN_EVALUAR"); break;
      case EH_OK: printf(diagnostics_putc,"OK"); break;
      case EH_DEGRADED: printf(diagnostics_putc,"DEGRADADO"); break;
      case EH_FAULT: printf(diagnostics_putc,"FALLO"); break;
      default: printf(diagnostics_putc,"DESHABILITADO"); break;
   }
}
static void event_diag_module(ev_u8 module)
{
   switch(module) {
      case EM_SYSTEM: printf(diagnostics_putc,"SISTEMA"); break;
      case EM_RTC: printf(diagnostics_putc,"RTC"); break;
      case EM_EEPROM: printf(diagnostics_putc,"EEPROM"); break;
      case EM_LORA: printf(diagnostics_putc,"LORA"); break;
      case EM_SD: printf(diagnostics_putc,"MICROSD"); break;
      case EM_ADC: printf(diagnostics_putc,"ADS1115"); break;
      case EM_RS485: printf(diagnostics_putc,"RS485"); break;
      default: printf(diagnostics_putc,"ALIMENTACION"); break;
   }
}
static void event_diag_storage(void)
{
   switch(event_storage_state) {
      case STORAGE_SCANNING: printf(diagnostics_putc,"EXPLORANDO_HISTORIAL"); break;
      case STORAGE_READY: printf(diagnostics_putc,"LISTO"); break;
      case STORAGE_ERROR: printf(diagnostics_putc,"ERROR_REINTENTO_5s"); break;
      case STORAGE_DISABLED: printf(diagnostics_putc,"DESHABILITADO"); break;
      case STORAGE_READING: printf(diagnostics_putc,"LEYENDO"); break;
      default: printf(diagnostics_putc,"ESCRIBIENDO_VERIFICANDO"); break;
   }
}
static void event_diag_code(ev_u16 code)
{
   switch(code) {
      case EV_SYSTEM_BOOT: printf(diagnostics_putc,"SYSTEM_BOOT"); break;
      case EV_RESET_WATCHDOG: printf(diagnostics_putc,"RESET_WATCHDOG"); break;
      case EV_RESET_BROWNOUT: printf(diagnostics_putc,"RESET_BROWNOUT"); break;
      case EV_RESET_EXTERNAL: printf(diagnostics_putc,"RESET_EXTERNAL"); break;
      case EV_RESET_SOFTWARE: printf(diagnostics_putc,"RESET_SOFTWARE"); break;
      case EV_RESET_STACK_FAULT: printf(diagnostics_putc,"RESET_STACK_FAULT"); break;
      case EV_RESET_UNKNOWN: printf(diagnostics_putc,"RESET_UNKNOWN"); break;
      case EV_RTC_FAULT: printf(diagnostics_putc,"RTC_FAULT"); break;
      case EV_RTC_RECOVERED: printf(diagnostics_putc,"RTC_RECOVERED"); break;
      case EV_RTC_TIME_INVALID: printf(diagnostics_putc,"RTC_TIME_INVALID"); break;
      case EV_EEPROM_FAULT: printf(diagnostics_putc,"EEPROM_FAULT"); break;
      case EV_EEPROM_RECOVERED: printf(diagnostics_putc,"EEPROM_RECOVERED"); break;
      case EV_EEPROM_CRC_ERROR: printf(diagnostics_putc,"EEPROM_CRC_ERROR"); break;
      case EV_LORA_FAULT: printf(diagnostics_putc,"LORA_FAULT"); break;
      case EV_LORA_RECOVERED: printf(diagnostics_putc,"LORA_RECOVERED"); break;
      case EV_SD_FAULT: printf(diagnostics_putc,"SD_FAULT"); break;
      case EV_SD_RECOVERED: printf(diagnostics_putc,"SD_RECOVERED"); break;
      case EV_ADC_FAULT: printf(diagnostics_putc,"ADC_FAULT"); break;
      case EV_ADC_RECOVERED: printf(diagnostics_putc,"ADC_RECOVERED"); break;
      case EV_RS485_COMM_LOST: printf(diagnostics_putc,"RS485_COMM_LOST"); break;
      case EV_RS485_COMM_RECOVERED: printf(diagnostics_putc,"RS485_COMM_RECOVERED"); break;
      case EV_POWER_LOW: printf(diagnostics_putc,"POWER_LOW"); break;
      case EV_POWER_HIGH: printf(diagnostics_putc,"POWER_HIGH"); break;
      case EV_POWER_RECOVERED: printf(diagnostics_putc,"POWER_RECOVERED"); break;
      default: printf(diagnostics_putc,"EVENTO_DESCONOCIDO"); break;
   }
}

ev_bool event_diagnostics_command(char *command)
{
   ev_u8 i;
   ev_u16 index=0;
   if(!strcmp(command,"LOG STATUS")) {
      printf(diagnostics_putc,"EEPROM "); event_diag_storage();
      printf(diagnostics_putc," registros=%Lu cola=%u perdidos=%LU errores=%LU\r\n",
         event_storage_count,event_queue_count,event_queue_lost,event_storage_errors);
   }
   else if(!strcmp(command,"LOG COUNT")) printf(diagnostics_putc,"LOG count=%Lu\r\n",event_manager_count());
   else if(!strcmp(command,"LOG CLEAR"))
      printf(diagnostics_putc,"LOG clear=%u (1=accepted; poll STATUS)\r\n",event_storage_clear());
   else if(!strncmp(command,"LOG READ ",9)) {
      if(!command[9]) { printf(diagnostics_putc,"LOG invalid index\r\n"); return 1; }
      for(i=9;command[i];i++) {
         if(command[i]<'0' || command[i]>'9' || index>153) {
            printf(diagnostics_putc,"LOG invalid index\r\n"); return 1;
         }
         index=index*10+command[i]-'0';
      }
      if(event_diag_reading || !event_manager_read(index)) printf(diagnostics_putc,"LOG busy/range\r\n");
      else event_diag_reading=1;
   }
   else if(!strcmp(command,"HEALTH STATUS")) {
      event_diag_health=0;
      printf(diagnostics_putc,"SALUD global="); event_diag_state(event_manager_global_status());
      printf(diagnostics_putc," (detalle por modulo a continuacion)\r\n");
   }
   else return 0;
   return 1;
}
void event_diagnostics_task(void)
{
   ev_u8 i;
   ev_u8 state;
   if(((diagnostics_tx_head-diagnostics_tx_tail)&127)>16) return;
   if(!event_diag_started) {
      event_diag_started=1;
      printf(diagnostics_putc,"EVENTOS: EEPROM 0x50, 1536 registros; recuperacion incremental. PCON0=0x%02X\r\n",event_reset_raw);
      return;
   }
   if(event_diag_health<EM_COUNT) {
      event_manager_get_status(event_diag_health,&state);
      printf(diagnostics_putc,"SALUD "); event_diag_module(event_diag_health++);
      printf(diagnostics_putc,"="); event_diag_state(state); printf(diagnostics_putc,"\r\n"); return;
   }
   if(event_storage_state==STORAGE_ERROR && event_diag_last_state!=STORAGE_ERROR) {
      event_diag_last_state=STORAGE_ERROR;
      printf(diagnostics_putc,"EEPROM FALLO: escritura/lectura no confirmada; reintento en 5 s. Revisar bus/WP.\r\n"); return;
   }
   if(event_storage_state==STORAGE_READY && (!event_diag_scan_reported || event_diag_last_state==STORAGE_ERROR)) {
      event_diag_scan_reported=1; event_diag_last_state=STORAGE_READY;
      printf(diagnostics_putc,"EEPROM LISTA: registros=%Lu boot=%Lu scan_ms=%LU corruptos=%LU\r\n",
         event_storage_count,event_storage_boot,event_scan_ms,event_storage_corrupt); return;
   }
   if(!event_diag_reading || !event_storage_read_done) return;
   if(!event_storage_read_ok) {
      printf(diagnostics_putc,"LOG ERROR: lectura I2C o CRC invalido; registro no entregado.\r\n");
      event_diag_reading=0; event_diag_detail=0; return;
   }
   if(event_diag_detail==0) {
      printf(diagnostics_putc,"LOG ");
      for(i=0;i<32;i++) printf(diagnostics_putc,"%02X",event_storage_read_data[i]);
      printf(diagnostics_putc,"\r\n");
   } else if(event_diag_detail==1) {
      printf(diagnostics_putc,"EVENTO seq=%LU boot=%Lu ",event_get_le(event_storage_read_data,4),(ev_u16)event_get_le(event_storage_read_data+26,2));
      event_diag_code((ev_u16)event_get_le(event_storage_read_data+12,2));
      printf(diagnostics_putc," CRC=OK\r\n");
   } else {
      printf(diagnostics_putc,"EVENTO uptime_s=%LU UTC=%LU flags=0x%02X p1=%LU p2=%LU\r\n",
         event_get_le(event_storage_read_data+8,4),event_get_le(event_storage_read_data+4,4),event_storage_read_data[17],
         event_get_le(event_storage_read_data+18,4),event_get_le(event_storage_read_data+22,4));
      event_diag_reading=0; event_diag_detail=0; return;
   }
   ++event_diag_detail;
}
