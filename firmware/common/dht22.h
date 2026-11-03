#ifndef DHT22_CAPTURE_H
#define DHT22_CAPTURE_H
// PIC18F67K40: RA2 (DI_1WIRE), INT0 y Timer1 a 2 MHz.
#define DHT22_PENDING 0
#define DHT22_OK 1
#define DHT22_TIMEOUT 2
#define DHT22_CHECKSUM 3
#define DHT22_RANGE 4
#define DHT22_PULSE 5
#define DHT22_BUS_LOW 6
void dht22_init(void);
int1 dht22_start(void);
void dht22_task(void);
int1 dht22_busy(void);
unsigned int8 dht22_result(void);
signed int16 dht22_temperature_x10;
unsigned int16 dht22_humidity_x10;
#endif

