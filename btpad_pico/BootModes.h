#pragma once
#include "Config.h"
struct BootMode { uint8_t gpio; HidMode mode; };
constexpr BootMode BOOT_MODES[] = {
  {14, MODE_WEB}, {6, MODE_GAMEPAD}, {7, MODE_SWITCH}, {8, MODE_PS3},
  {9, MODE_KEYBOARD}, {10, MODE_XBOX_BLE}, {11, MODE_DIRECTINPUT}, {12, MODE_XBOX_CLASSIC}
};
template<class PinReader>
HidMode selectBootMode(HidMode saved, PinReader held) {
  for (const auto &entry : BOOT_MODES)
    if (held(entry.gpio) && (entry.mode == MODE_WEB || isSelectableMode(entry.mode))) return entry.mode;
  return saved;
}
