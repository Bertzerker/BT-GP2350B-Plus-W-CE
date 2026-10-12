#pragma once
#include "Config.h"

struct PinDef {
  InputId id;
  uint8_t gpio;
  const char *name;
};

const PinDef PIN_MAP[] = {
  {IN_UP, 2, "UP"},
  {IN_DOWN, 3, "DOWN"},
  {IN_RIGHT, 4, "RIGHT"},
  {IN_LEFT, 5, "LEFT"},
  {IN_SOUTH, 6, "South"},
  {IN_EAST, 7, "East"},
  {IN_WEST, 10, "West"},
  {IN_NORTH, 11, "North"},
  {IN_L1, 13, "L1"},
  {IN_R1, 12, "R1"},
  {IN_L2, 9, "L2"},
  {IN_R2, 8, "R2"},
  {IN_SELECT, 17, "Select"},
  {IN_START, 16, "START"},
  {IN_LS, 18, "LS"},
  {IN_RS, 19, "RS"},
  {IN_SPECIAL, 14, "SPECIAL"},
  {IN_GUIDE, 20, "GUIDE"},
  {IN_CAPTURE, 21, "CAPTURE"}
};

const char *MODE_NAMES[] = {
  "Generic HID", "Switch (unimplemented)", "Reserved", "PS3 USB (experimental)", "DirectInput", "Keyboard", "WLAN", "Xbox BLE (experimental)", "Xbox Classic BT (experimental)"
};

// Protocol-independent destinations. Numeric IDs remain EEPROM-compatible.
const char *const LOGICAL_LABELS[IN_COUNT] = {
  "Up", "Down", "Right", "Left", "South", "East", "West", "North",
  "L1", "R1", "L2", "R2", "Select", "Start", "LS", "RS", "Special", "Guide", "Capture"
};
