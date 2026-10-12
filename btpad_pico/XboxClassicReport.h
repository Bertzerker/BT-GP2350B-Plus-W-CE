#pragma once
#include "XboxReport.h"

// Xbox Classic firmware 4.8 uses AC Back in byte 15 for Select, instead of
// the BLE Share usage. There is no native Capture button in this family.
inline XboxReport encodeXboxClassic(uint32_t state) {
  XboxReport report = encodeXbox(state & ~((1u << IN_SELECT) | (1u << IN_CAPTURE)));
  report.bytes[15] = (state & (1u << IN_SELECT)) ? 1 : 0;
  return report;
}

// Retain press edges until CAN_SEND_NOW. Transport calls these under its lock.
struct XboxClassicInput {
  uint32_t latest = 0, previous = 0, pending = 0;
  void clear() { latest = previous = pending = 0; }
  void update(uint32_t state) { latest = state; pending |= state & ~previous; previous = state; }
  XboxReport current() const { return encodeXboxClassic(latest | pending); }
  void sent() { pending = 0; }
};
