/* sd_card.c was included with these four public names temporarily renamed.
 * Wrappers observe actual completed operations including FAT callers. */
int1 sd_init(void)
{
   int1 ok=event_sd_driver_init();
   event_health_sd(ok,sd_error,0xFFFFFFFFUL); return ok;
}
int1 sd_read_sector(unsigned int32 sector,unsigned int8 *buffer)
{
   int1 ok=event_sd_driver_read(sector,buffer);
   event_health_sd(ok,sd_error,sector); return ok;
}
int1 sd_write_sector(unsigned int32 sector,unsigned int8 *buffer)
{
   int1 ok=event_sd_driver_write(sector,buffer);
   event_health_sd(ok,sd_error,sector); return ok;
}
int1 sd_sync(void)
{
   int1 ok=event_sd_driver_sync();
   event_health_sd(ok,sd_error,0xFFFFFFFFUL); return ok;
}
