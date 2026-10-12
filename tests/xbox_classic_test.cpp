#include "../btpad_pico/XboxClassicReport.h"
#include "../btpad_pico/XboxClassicDescriptor.h"
#include <assert.h>
#include <stdio.h>
int main() {
  unsigned sizes[256] = {}, outputs[256] = {}, id = 0, count = 0, bits = 0;
  int collections = 0;
  for (size_t pos = 0; pos < sizeof(xboxClassicDescriptor);) {
    const uint8_t tag = xboxClassicDescriptor[pos++];
    const unsigned length = (tag & 3) == 3 ? 4 : tag & 3;
    assert(pos + length <= sizeof(xboxClassicDescriptor));
    unsigned value = 0;
    for (unsigned i = 0; i < length; ++i) value |= unsigned(xboxClassicDescriptor[pos++]) << (i * 8);
    switch (tag & 0xfc) {
      case 0xa0: ++collections; break; case 0xc0: --collections; break;
      case 0x84: id = value; break; case 0x94: count = value; break; case 0x74: bits = value; break;
      case 0x80: sizes[id] += bits * count; break; case 0x90: outputs[id] += bits * count; break;
    }
    assert(collections >= 0);
  }
  assert(collections == 0 && sizes[1] == 128 && sizes[2] == 8 && sizes[4] == 8 && outputs[3] == 64);
  const auto neutral = encodeXboxClassic(0);
  for (int i = 0; i < 4; ++i) assert(neutral.bytes[2*i] == 0 && neutral.bytes[2*i+1] == 128);
  for (int i = 8; i < 16; ++i) assert(neutral.bytes[i] == 0);
  const auto triggers = encodeXboxClassic((1u << IN_L2) | (1u << IN_R2));
  assert(triggers.bytes[8] == 255 && triggers.bytes[9] == 3 && triggers.bytes[10] == 255 && triggers.bytes[11] == 3);
  assert(triggers.bytes[13] == 0 && triggers.bytes[14] == 0 && triggers.bytes[15] == 0);
  const auto back = encodeXboxClassic(1u << IN_SELECT);
  assert(back.bytes[13] == 0 && back.bytes[14] == 0 && back.bytes[15] == 1);
  const auto capture = encodeXboxClassic(1u << IN_CAPTURE);
  assert(memcmp(&capture, &neutral, sizeof(neutral)) == 0); // No Share in this family.
  const auto guide = encodeXboxClassic(1u << IN_GUIDE);
  assert(guide.bytes[13] == 0 && guide.bytes[14] == 0x10 && guide.bytes[15] == 0);
  const uint32_t directions[] = {1,5,4,6,2,10,8,9};
  for (uint8_t i = 0; i < 8; ++i) assert(encodeXboxClassic(directions[i]).bytes[12] == i + 1);
  XboxClassicInput queue;
  queue.update(1u << IN_GUIDE); queue.update(0);
  assert(queue.current().bytes[14] == 0x10); // Short tap survives transport backpressure.
  queue.sent(); assert(queue.current().bytes[14] == 0);
  queue.update(1u << IN_SOUTH); queue.sent(); assert(queue.current().bytes[13] == 1);
  queue.clear(); assert(queue.current().bytes[13] == 0); // No stuck input on reconnect.
  puts("Xbox Classic: descriptor, Back/Guide, triggers, hat, Capture isolation and queued taps passed.");
}
