#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum HidMode : uint8_t {
  MODE_GAMEPAD = 0,
  MODE_SWITCH = 1,
  MODE_RESERVED_2 = 2,
  MODE_PS3 = 3,
  MODE_DIRECTINPUT = 4,
  MODE_KEYBOARD = 5,
  MODE_WEB = 6,
  MODE_XBOX_BLE = 7 // Experimental Windows XInput-compatible BLE transport.
};

enum SocdMode : uint8_t {
  SOCD_NEUTRAL = 0,
  SOCD_UP_PRIORITY = 1,
  SOCD_LAST_INPUT = 2
};

inline bool isSelectableMode(HidMode mode) {
  return mode == MODE_PS3 || mode == MODE_GAMEPAD || mode == MODE_DIRECTINPUT || mode == MODE_KEYBOARD || mode == MODE_XBOX_BLE;
}

inline HidMode migrateLegacyMode(HidMode mode) {
  return mode == MODE_SWITCH || mode == MODE_RESERVED_2 ? MODE_GAMEPAD : mode;
}

enum InputId : uint8_t {
  IN_UP,
  IN_DOWN,
  IN_RIGHT,
  IN_LEFT,
  IN_SOUTH,
  IN_EAST,
  IN_WEST,
  IN_NORTH,
  IN_L1,
  IN_R1,
  IN_L2,
  IN_R2,
  IN_SELECT,
  IN_START,
  IN_LS,
  IN_RS,
  IN_SPECIAL,
  IN_GUIDE,
  IN_CAPTURE,
  IN_COUNT
};

struct Profile {
  char name[20];
  uint8_t remap[IN_COUNT];
  uint16_t turboMask;
  uint8_t turboHz;
  SocdMode socd;
};

struct Config {
  uint32_t magic;
  char adminPassword[24];
  HidMode defaultMode;
  uint8_t activeProfile;
  Profile profiles[3];
};

constexpr uint32_t CONFIG_MAGIC = 0x42545044; // BTPD

inline bool profileExists(const Config &value, uint8_t slot) {
  return slot < 3 && value.profiles[slot].name[0] != '\0';
}
inline uint8_t nextProfile(const Config &value, uint8_t from, bool forward) {
  for (uint8_t n = 1; n <= 3; ++n) {
    uint8_t slot = (from + (forward ? n : 3 - n)) % 3;
    if (profileExists(value, slot)) return slot;
  }
  return from;
}
inline void resetProfile(Profile &profile, const char *name) {
  memset(&profile, 0, sizeof(profile));
  for (size_t i = 0; i < sizeof(profile.name) - 1 && name[i]; ++i) profile.name[i] = name[i];
  for (uint8_t i = 0; i < IN_COUNT; ++i) profile.remap[i] = i;
  profile.turboHz = 12;
}

// Keep the existing EEPROM layout; a versioned format is a separate migration.
inline bool isConfigValid(const Config &candidate) {
  if (candidate.magic != CONFIG_MAGIC || (candidate.defaultMode >= MODE_WEB && candidate.defaultMode != MODE_XBOX_BLE) ||
      !profileExists(candidate, candidate.activeProfile)) return false;

  const char *passwordEnd = static_cast<const char *>(
      memchr(candidate.adminPassword, '\0', sizeof(candidate.adminPassword)));
  if (!passwordEnd || passwordEnd - candidate.adminPassword < 8) return false;

  // The current 16-bit schema supports turbo only for A through RS.
  constexpr uint16_t allowedTurboMask = 0xFFF0;
  for (const Profile &profile : candidate.profiles) {
    if (!memchr(profile.name, '\0', sizeof(profile.name)) ||
        profile.turboHz < 2 || profile.turboHz > 30 ||
        profile.socd > SOCD_LAST_INPUT ||
        (profile.turboMask & ~allowedTurboMask) != 0) return false;
    for (uint8_t physical = 0; physical < IN_COUNT; ++physical) {
      if (profile.remap[physical] >= IN_COUNT) return false;
      if (physical == IN_SPECIAL) {
        if (profile.remap[physical] != IN_SPECIAL) return false;
      } else if (profile.remap[physical] == IN_SPECIAL) {
        return false;
      }
    }
  }
  return true;
}
