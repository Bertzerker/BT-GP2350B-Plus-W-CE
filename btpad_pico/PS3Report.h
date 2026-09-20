#pragma once
#include "Config.h"
#include "PS3Descriptor.h"

struct PS3InputReport { uint8_t bytes[49]; };
inline PS3InputReport encodePS3(uint32_t logical) {
  PS3InputReport out = {};
  out.bytes[0] = 1;
  const InputId buttons[] = {IN_SELECT, IN_LS, IN_RS, IN_START, IN_UP, IN_RIGHT, IN_DOWN, IN_LEFT,
    IN_L2, IN_R2, IN_L1, IN_R1, IN_NORTH, IN_EAST, IN_SOUTH, IN_WEST, IN_GUIDE};
  for (uint8_t i = 0; i < 17; ++i)
    if (logical & (1u << buttons[i])) out.bytes[2 + i / 8] |= 1u << (i % 8);
  for (uint8_t i = 6; i < 10; ++i) out.bytes[i] = 0x80;
  const InputId pressure[] = {IN_UP, IN_RIGHT, IN_DOWN, IN_LEFT, IN_L2, IN_R2, IN_L1, IN_R1, IN_NORTH, IN_EAST, IN_SOUTH, IN_WEST};
  for (uint8_t i = 0; i < 12; ++i) out.bytes[14 + i] = (logical & (1u << pressure[i])) ? 255 : 0;
  out.bytes[29] = 2; out.bytes[30] = 0xee; out.bytes[31] = 0x12; // USB powered.
  // Fixed neutral motion readings; this board has no motion sensor.
  out.bytes[41] = out.bytes[43] = out.bytes[45] = out.bytes[47] = 2;
  return out;
}

class PS3Features {
public:
  uint8_t deviceAddress[6] = {}, hostAddress[6] = {};
  uint8_t efByte = 0xa0;
  uint16_t get(uint8_t id, uint8_t *buffer, uint16_t requested) const {
    const uint8_t *source = nullptr; size_t length = 0;
    uint8_t pairing[17] = {0xff, 0xff};
    switch (id) {
      case 1: source = output_ps3_0x01; length = sizeof(output_ps3_0x01); break;
      case 0xef: source = output_ps3_0xef; length = sizeof(output_ps3_0xef); break;
      case 0xf2:
        memcpy(pairing + 3, deviceAddress, 6); memcpy(pairing + 10, hostAddress, 6);
        source = pairing; length = sizeof(pairing); break;
      case 0xf5: source = output_ps3_0xf5; length = sizeof(output_ps3_0xf5); break;
      case 0xf7: source = output_ps3_0xf7; length = sizeof(output_ps3_0xf7); break;
      case 0xf8: source = output_ps3_0xf8; length = sizeof(output_ps3_0xf8); break;
      default: return 0; // Unsupported feature requests stall.
    }
    uint16_t count = static_cast<uint16_t>(length < requested ? length : requested);
    memcpy(buffer, source, count);
    if ((id == 0xef || id == 0xf8) && count > 6) buffer[6] = efByte;
    if (id == 0xf5) for (uint8_t i = 0; i < 6 && i + 1 < count; ++i) buffer[i+1] = hostAddress[i];
    return count;
  }
  void set(uint8_t id, const uint8_t *buffer, uint16_t length) {
    if (id == 0xef && length > 6) efByte = buffer[6];
    if (id == 0xf5 && length >= 7) memcpy(hostAddress, buffer + 1, 6);
  }
};
