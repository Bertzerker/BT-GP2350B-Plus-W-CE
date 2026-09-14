#pragma once
#include "Config.h"

struct GamepadOutput {
  int x = 0, y = 0, z = -32767, rz = -32767;
  int hatAngle = -1;
  uint32_t buttons = 0;
};

inline GamepadOutput encodeGamepad(uint32_t state, HidMode mode) {
  GamepadOutput out;
  const auto down = [state](InputId id) { return (state & (1u << id)) != 0; };
  const int x = int(down(IN_RIGHT)) - int(down(IN_LEFT));
  const int y = int(down(IN_DOWN)) - int(down(IN_UP));
  if (mode == MODE_DIRECTINPUT) {
    // Preserve the user-verified legacy DirectInput report.
    out.x = x * 32767;
    out.y = y * 32767;
  } else {
    // One directional source: the HID hat, not duplicated stick axes.
    if (y < 0) out.hatAngle = x < 0 ? 315 : (x > 0 ? 45 : 0);
    else if (y > 0) out.hatAngle = x < 0 ? 225 : (x > 0 ? 135 : 180);
    else if (x) out.hatAngle = x < 0 ? 270 : 90;
  }
  out.z = down(IN_LT) ? 32767 : -32767;
  out.rz = down(IN_RT) ? 32767 : -32767;
  const InputId buttons[] = {IN_A, IN_B, IN_X, IN_Y, IN_LB, IN_RB, IN_LT,
      IN_RT, IN_BACK, IN_START, IN_LS, IN_RS, IN_GUIDE, IN_CAPTURE};
  for (uint8_t i = 0; i < 14; ++i) {
    if (mode != MODE_DIRECTINPUT && (buttons[i] == IN_LT || buttons[i] == IN_RT)) continue;
    if (down(buttons[i])) out.buttons |= 1u << i;
  }
  return out;
}
