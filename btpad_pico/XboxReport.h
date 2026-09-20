#pragma once
#include "GamepadOutput.h"
struct XboxReport { uint8_t bytes[16]; };
inline XboxReport encodeXbox(uint32_t state) {
  XboxReport out = {};
  for (int i = 0; i < 4; ++i) out.bytes[i * 2 + 1] = 0x80; // Both sticks centered.
  const auto held = [state](InputId id) { return (state & (1u << id)) != 0; };
  if (held(IN_L2)) { out.bytes[8] = 0xff; out.bytes[9] = 3; }
  if (held(IN_R2)) { out.bytes[10] = 0xff; out.bytes[11] = 3; }
  const int hat = encodeGamepad(state, MODE_GAMEPAD).hatAngle;
  out.bytes[12] = hat < 0 ? 0 : static_cast<uint8_t>(hat / 45 + 1);
  const InputId ids[] = {IN_SOUTH,IN_EAST,IN_WEST,IN_NORTH,IN_L1,IN_R1,IN_SELECT,IN_START,IN_GUIDE,IN_LS,IN_RS};
  const uint16_t bits[] = {1,2,8,16,64,128,0x400,0x800,0x1000,0x2000,0x4000};
  uint16_t buttons = 0;
  for (uint8_t i=0;i<11;++i) if (held(ids[i])) buttons |= bits[i];
  out.bytes[13] = buttons & 0xff; out.bytes[14] = buttons >> 8;
  out.bytes[15] = held(IN_CAPTURE) ? 1 : 0;
  return out;
}
