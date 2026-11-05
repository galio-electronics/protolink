/* Shared bounded I2C1 port, also used by the event EEPROM. */
#include "../../common/station_i2c.c"
int1 rtc_bus_write(unsigned int8 reg, unsigned int8 *data, unsigned int8 n)
{ return station_i2c_write(0xDE,reg,1,data,n); }
int1 rtc_bus_read(unsigned int8 reg, unsigned int8 *data, unsigned int8 n)
{ return station_i2c_read(0xDE,reg,1,data,n); }
