#include "../btpad_pico/XboxReport.h"
#include "../btpad_pico/XboxDescriptor.h"
#include <assert.h>
#include <stdio.h>
int main() {
  const auto neutral = encodeXbox(0);
  assert(sizeof(neutral)==16);
  for(int i=0;i<4;++i) assert(neutral.bytes[i*2]==0 && neutral.bytes[i*2+1]==128);
  for(int i=8;i<16;++i) assert(neutral.bytes[i]==0);
  const auto all = encodeXbox((1u<<IN_SOUTH)|(1u<<IN_GUIDE)|(1u<<IN_L2)|(1u<<IN_R2)|(1u<<IN_UP)|(1u<<IN_RIGHT)|(1u<<IN_CAPTURE));
  assert(all.bytes[8]==255 && all.bytes[9]==3 && all.bytes[10]==255 && all.bytes[11]==3);
  assert(all.bytes[12]==2 && all.bytes[13]==1 && all.bytes[14]==16 && all.bytes[15]==1);
  // Parse report sizes from the actual descriptor, independent of the encoder.
  unsigned int sizes[256]={}, outputs[256]={}, report=0,count=0,bits=0;
  for(size_t pos=0;pos<sizeof(XboxOneS_1914_HIDDescriptor);) {
    const uint8_t tag=XboxOneS_1914_HIDDescriptor[pos++];
    const unsigned int length=(tag&3)==3 ? 4 : tag&3;
    unsigned int value=0; for(unsigned int i=0;i<length;++i) value|=unsigned(XboxOneS_1914_HIDDescriptor[pos++])<<(i*8);
    switch(tag&0xfc) {case 0x84:report=value;break;case 0x94:count=value;break;case 0x74:bits=value;break;case 0x80:sizes[report]+=bits*count;break;case 0x90:outputs[report]+=bits*count;break;}
  }
  assert(sizes[1]==128 && outputs[3]==64);
  puts("Xbox report passed: descriptor sizes, neutral, hat, separate triggers and Guide.");
}
