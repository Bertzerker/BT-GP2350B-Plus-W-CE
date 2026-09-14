#pragma once
#include <stdint.h>

// Input must already be debounced. Short presses are emitted on release so a
// pairing hold cannot also send Capture/Touchpad to the host.
class SyncButton {
public:
  bool update(bool pressed, uint32_t now, bool suppress = false) {
    if (suppress) { blocked = true; tapPending = false; }
    if (blocked) {
      wasPressed = pressed;
      fired = true;
      if (!pressed) blocked = false;
      return false;
    }
    if (pressed && !wasPressed) { started = now; fired = false; }
    bool toggle = false;
    if (pressed && !fired && static_cast<uint32_t>(now - started) >= 3000) {
      fired = true;
      toggle = true;
    }
    if (!pressed && wasPressed && !fired) tapPending = true;
    wasPressed = pressed;
    return toggle;
  }
  bool takeTap() {
    bool result = tapPending;
    tapPending = false;
    return result;
  }
private:
  bool wasPressed = false, fired = false, tapPending = false;
  bool blocked = false;
  uint32_t started = 0;
};
