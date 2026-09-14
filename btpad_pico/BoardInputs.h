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
  {IN_A, 6, "A"},
  {IN_B, 7, "B"},
  {IN_X, 10, "X"},
  {IN_Y, 11, "Y"},
  {IN_LB, 13, "LB"},
  {IN_RB, 12, "RB"},
  {IN_LT, 9, "LT"},
  {IN_RT, 8, "RT"},
  {IN_BACK, 17, "BACK"},
  {IN_START, 16, "START"},
  {IN_LS, 18, "LS"},
  {IN_RS, 19, "RS"},
  {IN_SPECIAL, 14, "SPECIAL"},
  {IN_GUIDE, 20, "GUIDE"},
  {IN_CAPTURE, 21, "CAPTURE"}
};

const char *MODE_NAMES[] = {
  "Generic HID", "Switch (unimplemented)", "Reserved", "PS3 (unimplemented)", "DirectInput", "Keyboard", "WLAN"
};

struct ButtonLabels {
  const char *labels[IN_COUNT];
};

const ButtonLabels MODE_LABELS[] = {
  {{"UP","DOWN","RIGHT","LEFT","A","B","X","Y","LB","RB","LT","RT","Back","Start","LS","RS","Special","Guide","-"}},
  {{"UP","DOWN","RIGHT","LEFT","B","A","Y","X","L","R","ZL","ZR","Minus","Plus","LS","RS","Special","Home","Capture"}},
  {{}}, // Retired mode slot; retained only to preserve stored IDs.
  {{"UP","DOWN","RIGHT","LEFT","Cross","Circle","Square","Triangle","L1","R1","L2","R2","Select","Start","L3","R3","Special","PS","-"}},
  {{"UP","DOWN","RIGHT","LEFT","2","3","1","4","5","6","7","8","9","10","11","12","Special","13","-"}},
  {{"UP","DOWN","RIGHT","LEFT","z","x","a","s","v","f","c","d","Backspace","Enter","b","g","Special","Esc","-"}}
};
