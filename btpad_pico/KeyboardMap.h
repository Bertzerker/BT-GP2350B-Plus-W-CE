#pragma once
#include "Config.h"

struct KeyOption { uint8_t usage; const char *name; };
const KeyOption KEY_OPTIONS[] = {
  {0,"Unassigned"},
  {4,"A"},{5,"B"},{6,"C"},{7,"D"},{8,"E"},{9,"F"},{10,"G"},{11,"H"},{12,"I"},{13,"J"},{14,"K"},{15,"L"},{16,"M"},
  {17,"N"},{18,"O"},{19,"P"},{20,"Q"},{21,"R"},{22,"S"},{23,"T"},{24,"U"},{25,"V"},{26,"W"},{27,"X"},{28,"Y"},{29,"Z"},
  {30,"1"},{31,"2"},{32,"3"},{33,"4"},{34,"5"},{35,"6"},{36,"7"},{37,"8"},{38,"9"},{39,"0"},
  {40,"Enter"},{41,"Escape"},{42,"Backspace"},{43,"Tab"},{44,"Space"},{45,"Minus"},{46,"Equals"},
  {47,"Left bracket"},{48,"Right bracket"},{49,"Backslash"},{50,"Non-US #"},{51,"Semicolon"},{52,"Quote"},{53,"Grave"},{54,"Comma"},{55,"Period"},{56,"Slash"},{57,"Caps Lock"},
  {58,"F1"},{59,"F2"},{60,"F3"},{61,"F4"},{62,"F5"},{63,"F6"},{64,"F7"},{65,"F8"},{66,"F9"},{67,"F10"},{68,"F11"},{69,"F12"},
  {70,"Print Screen"},{71,"Scroll Lock"},{72,"Pause"},{73,"Insert"},{74,"Home"},{75,"Page Up"},{76,"Delete"},{77,"End"},{78,"Page Down"},
  {79,"Right arrow"},{80,"Left arrow"},{81,"Down arrow"},{82,"Up arrow"},{83,"Num Lock"},
  {84,"Keypad /"},{85,"Keypad *"},{86,"Keypad -"},{87,"Keypad +"},{88,"Keypad Enter"},
  {89,"Keypad 1"},{90,"Keypad 2"},{91,"Keypad 3"},{92,"Keypad 4"},{93,"Keypad 5"},{94,"Keypad 6"},{95,"Keypad 7"},{96,"Keypad 8"},{97,"Keypad 9"},{98,"Keypad 0"},{99,"Keypad ."},
  {100,"Non-US backslash"},{101,"Menu / Application"},{102,"Power"},{103,"Keypad ="},
  {104,"F13"},{105,"F14"},{106,"F15"},{107,"F16"},{108,"F17"},{109,"F18"},{110,"F19"},{111,"F20"},{112,"F21"},{113,"F22"},{114,"F23"},{115,"F24"},
  {135,"International 1"},{136,"International 2"},{137,"International 3"},{138,"International 4"},{139,"International 5"},
  {140,"International 6"},{141,"International 7"},{142,"International 8"},{143,"International 9"},
  {144,"Language 1"},{145,"Language 2"},{146,"Language 3"},{147,"Language 4"},{148,"Language 5"},
  {224,"Left Ctrl"},{225,"Left Shift"},{226,"Left Alt"},{227,"Left Windows / GUI"},
  {228,"Right Ctrl"},{229,"Right Shift"},{230,"Right Alt / AltGr"},{231,"Right Windows / GUI"}
};
inline bool validKey(uint8_t usage) {
  for (const auto &key : KEY_OPTIONS) if (key.usage == usage) return true;
  return false;
}

// Separate versioned extension after the unchanged legacy Config in EEPROM.
struct KeyboardConfig {
  uint32_t magic;
  uint8_t version;
  uint8_t maps[3][IN_COUNT];
  uint16_t reserved;
  uint32_t checksum;
};
constexpr uint32_t KEYBOARD_MAGIC = 0x4b4d4150;
inline uint32_t keyboardChecksum(const KeyboardConfig &value) {
  uint32_t hash = 2166136261u;
  hash = (hash ^ value.version) * 16777619u;
  for (const auto &map : value.maps) for (uint8_t key : map) hash = (hash ^ key) * 16777619u;
  return hash;
}
inline bool validKeyboardConfig(const KeyboardConfig &value) {
  if (value.magic != KEYBOARD_MAGIC || value.version != 1 || value.reserved != 0 || value.checksum != keyboardChecksum(value)) return false;
  for (const auto &map : value.maps) {
    if (map[IN_SPECIAL] != 0) return false;
    for (uint8_t key : map) if (!validKey(key)) return false;
  }
  return true;
}
inline KeyboardConfig defaultKeyboardConfig() {
  KeyboardConfig result = {};
  result.magic = KEYBOARD_MAGIC; result.version = 1;
  const uint8_t defaults[IN_COUNT] = {82,81,79,80,29,27,4,22,25,9,6,7,42,40,5,10,0,41,43};
  for (auto &map : result.maps) memcpy(map, defaults, sizeof(defaults));
  result.checksum = keyboardChecksum(result);
  return result;
}
struct KeyboardReport { uint8_t modifiers, reserved, keys[6]; };
inline KeyboardReport encodeKeyboard(uint32_t physical, const uint8_t map[IN_COUNT]) {
  KeyboardReport result = {};
  uint8_t count = 0;
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    if (i == IN_SPECIAL || !(physical & (1u << i))) continue;
    const uint8_t key = map[i];
    if (!key) continue;
    if (key >= 224 && key <= 231) { result.modifiers |= 1u << (key - 224); continue; }
    bool duplicate = false;
    for (uint8_t j = 0; j < count && j < 6; ++j) if (result.keys[j] == key) duplicate = true;
    if (!duplicate) { if (count < 6) result.keys[count] = key; ++count; }
  }
  if (count > 6) memset(result.keys, 1, sizeof(result.keys)); // HID ErrorRollOver, never stale keys.
  return result;
}
