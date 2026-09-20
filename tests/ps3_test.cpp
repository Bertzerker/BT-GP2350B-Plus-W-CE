#include "../btpad_pico/PS3Report.h"
#include <assert.h>
#include <stdio.h>
int main() {
  const auto neutral = encodePS3(0);
  assert(sizeof(neutral) == 49 && neutral.bytes[0] == 1);
  for (int i = 6; i < 10; ++i) assert(neutral.bytes[i] == 128);
  auto out = encodePS3((1u << IN_UP) | (1u << IN_RIGHT) | (1u << IN_SOUTH) | (1u << IN_L2) | (1u << IN_R2) | (1u << IN_GUIDE));
  assert(out.bytes[2] == 0x30 && out.bytes[3] == 0x43 && out.bytes[4] == 1);
  assert(out.bytes[14] == 255 && out.bytes[15] == 255 && out.bytes[18] == 255 && out.bytes[19] == 255 && out.bytes[24] == 255);
  assert(ps3_report_descriptor[6] == 0x85 && ps3_report_descriptor[7] == 1);
  unsigned report = 0, count = 0, bits = 0, inputBits = 0;
  int collections = 0;
  for (size_t pos = 0; pos < sizeof(ps3_report_descriptor);) {
    uint8_t tag = ps3_report_descriptor[pos++]; unsigned len = (tag & 3) == 3 ? 4 : tag & 3, value = 0;
    for (unsigned i = 0; i < len; ++i) value |= unsigned(ps3_report_descriptor[pos++]) << (8 * i);
    switch (tag & 0xfc) {
      case 0xa0: ++collections; break; case 0xc0: --collections; break;
      case 0x84: report = value; break; case 0x94: count = value; break; case 0x74: bits = value; break;
      case 0x80: if (report == 1) inputBits += count * bits; break;
    }
    assert(collections >= 0);
  }
  assert(collections == 0 && inputBits == 48 * 8);
  PS3Features feature;
  uint8_t buffer[80];
  for (unsigned length = 0; length <= 70; ++length) {
    memset(buffer, 0xcc, sizeof(buffer));
    uint16_t copied = feature.get(0xef, buffer, static_cast<uint16_t>(length));
    assert(copied <= length && copied <= sizeof(output_ps3_0xef));
    assert(buffer[copied] == 0xcc);
  }
  memset(buffer, 0, sizeof(buffer)); buffer[6] = 0x55;
  feature.set(0xef, buffer, 6); assert(feature.efByte == 0xa0);
  feature.set(0xef, buffer, 7); feature.get(0xf8, buffer, 7); assert(buffer[6] == 0x55);
  assert(feature.get(0x99, buffer, 64) == 0);
  puts("PS3 USB reports passed: descriptor, buttons, pressures, Guide and bounded feature responses.");
}
