#pragma once
#include <stdint.h>

// DS3 output report 1: LED bitmap at wire byte 10 (payload byte 9).
// Format reference: Linux drivers/hid/hid-sony.c, sixaxis_output_report.
// Zero means unknown, including all LEDs on during pairing. Host LED order
// need not match the player order chosen independently by a game.
inline bool decodePS3Player(uint8_t id, const uint8_t *data, uint16_t size, uint8_t &player) {
  if (!data) return false;
  // TinyUSB passes interrupt OUT with the ID included; control SET_REPORT
  // supplies the ID separately and strips it from the payload.
  if (id == 0) {
    if (!size || data[0] != 1) return false;
    id = *data++; --size;
  }
  if (id != 1 || size < 10) return false;
  switch (data[9]) {
    case 0x02: player = 1; break;
    case 0x04: player = 2; break;
    case 0x08: player = 3; break;
    case 0x10: player = 4; break;
    case 0x12: player = 5; break;
    case 0x14: player = 6; break;
    case 0x18: player = 7; break;
    default: player = 0; break;
  }
  return true;
}
