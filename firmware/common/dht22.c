#include "dht22.h"
#pin_select INT0=DI_1WIRE
volatile unsigned int16 dht22_intervals[42];
volatile unsigned int8 dht22_edge_count = 0;
volatile unsigned int16 dht22_previous_edge;
static unsigned int8 dht22_state = 0;
static unsigned int8 dht22_ready = DHT22_PENDING;
static unsigned int16 dht22_phase_started;
unsigned int8 dht22_start_edges;
int1 dht22_start_low;

#INT_EXT
void dht22_capture_isr(void)
{
   unsigned int16 captured = get_timer1();
   if(dht22_edge_count < 42)
   {
      dht22_intervals[dht22_edge_count] = captured - dht22_previous_edge;
      dht22_previous_edge = captured;
      ++dht22_edge_count;
      if(dht22_edge_count == 42) disable_interrupts(INT_EXT);
   }
}

void dht22_init(void)
{
   disable_interrupts(INT_EXT);
   ext_int_edge(0,H_TO_L);
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
   output_float(DI_1WIRE);
   dht22_state = 0;
   dht22_ready = DHT22_PENDING;
}

int1 dht22_busy(void) { return dht22_state != 0; }

static void dht22_finish(unsigned int8 result)
{
   disable_interrupts(INT_EXT);
   ext_int_edge(0,H_TO_L);
   output_float(DI_1WIRE);
   dht22_state = 0;
   dht22_ready = result;
}

int1 dht22_start(void)
{
   if(dht22_state) return FALSE;
   dht22_ready = DHT22_PENDING;
   if(!input(DI_1WIRE))
   {
      dht22_finish(DHT22_BUS_LOW);
      return FALSE;
   }
   // Verificar que el pulso propio llega a INT0 antes de recibir al sensor.
   dht22_edge_count = 0;
   dht22_previous_edge = get_timer1();
   clear_interrupt(INT_EXT);
   enable_interrupts(INT_EXT);
   // Open drain: solo conducir bajo, nunca forzar alto.
   output_low(DI_1WIRE);
   output_drive(DI_1WIRE);
   dht22_start_low = !input(DI_1WIRE);
   dht22_phase_started = get_timer1();
   dht22_state = 1;
   return TRUE;
}

void dht22_task(void)
{
   unsigned int8 data[5] = {0,0,0,0,0};
   unsigned int8 i, sum;
   unsigned int16 width, humidity, magnitude;
   signed int16 temperature;
   if(dht22_state == 1)
   {
      // Pulso de inicio >= 1 ms, sin bloquear el scheduler.
      if((unsigned int16)(get_timer1()-dht22_phase_started) < 2400) return;
      disable_interrupts(INT_EXT);
      dht22_start_edges = dht22_edge_count;
      dht22_edge_count = 0;
      dht22_previous_edge = get_timer1();
      ext_int_edge(0,H_TO_L);
      clear_interrupt(INT_EXT);
      enable_interrupts(INT_EXT);
      dht22_phase_started = get_timer1();
      output_float(DI_1WIRE);
      dht22_state = 2;
      return;
   }
   if(dht22_state != 2) return;
   if(dht22_edge_count < 42)
   {
      if((unsigned int16)(get_timer1()-dht22_phase_started) >= 24000)
         dht22_finish(DHT22_TIMEOUT);
      return;
   }
   // Entre caidas: preambulo 80+80 us; bits 50+26 us o 50+70 us.
   // Timer1 a 2 MHz: cada tick equivale a 0.5 us.
   width = dht22_intervals[1];
   if(width < 240 || width > 400)
   {
      dht22_finish(DHT22_PULSE);
      return;
   }
   for(i=0;i<40;i++)
   {
      width = dht22_intervals[i+2];
      if(width < 120 || width > 300)
      {
         dht22_finish(DHT22_PULSE);
         return;
      }
      data[i/8] = (data[i/8] << 1) | (width > 200);
   }
   sum = data[0]+data[1]+data[2]+data[3];
   if(sum != data[4])
   {
      dht22_finish(DHT22_CHECKSUM);
      return;
   }
   humidity = make16(data[0],data[1]);
   magnitude = make16(data[2] & 0x7F,data[3]);
   temperature = (signed int16)magnitude;
   if(bit_test(data[2],7)) temperature = -temperature;
   if(humidity > 1000 || temperature < -400 || temperature > 800)
   {
      dht22_finish(DHT22_RANGE);
      return;
   }
   dht22_humidity_x10 = humidity;
   dht22_temperature_x10 = temperature;
   dht22_finish(DHT22_OK);
}

unsigned int8 dht22_result(void)
{
   unsigned int8 result = dht22_ready;
   dht22_ready = DHT22_PENDING;
   return result;
}


