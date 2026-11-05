#include "event_crc.h"
ev_u16 event_crc16(ev_u8 *data, ev_u8 length)
{
   ev_u16 crc=0xFFFF;
   ev_u8 i,j;
   for(i=0;i<length;i++) {
      crc ^= (ev_u16)data[i]<<8;
      for(j=0;j<8;j++) crc=(crc & 0x8000) ? (crc<<1)^0x1021 : crc<<1;
   }
   return crc;
}
ev_u32 event_get_le(ev_u8 *data, ev_u8 length)
{
   ev_u32 value=0;
   ev_u8 i;
   for(i=0;i<length;i++) value |= (ev_u32)data[i]<<(8*i);
   return value;
}
void event_put_le(ev_u8 *data, ev_u32 value, ev_u8 length)
{
   ev_u8 i;
   for(i=0;i<length;i++) { data[i]=(ev_u8)value; value>>=8; }
}
ev_bool event_record_valid(ev_u8 *data)
{
   ev_u8 i;
   ev_bool erased=1,zero=1;
   for(i=0;i<32;i++) { if(data[i]!=255) erased=0; if(data[i]) zero=0; }
   return !erased && !zero && event_get_le(data+30,2)==event_crc16(data,30);
}
ev_bool event_sequence_after(ev_u32 a, ev_u32 b)
{
   ev_u32 delta=a-b;
   return delta!=0 && delta<0x80000000UL;
}
