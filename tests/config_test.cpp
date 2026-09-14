#include "../btpad_pico/Config.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>

Config validConfig() {
  Config value = {};
  value.magic = CONFIG_MAGIC;
  memcpy(value.adminPassword, "changeme", sizeof("changeme"));
  value.defaultMode = MODE_DIRECTINPUT;
  for (Profile &profile : value.profiles) {
    memcpy(profile.name, "Default", sizeof("Default"));
    profile.turboHz = 12;
    profile.socd = SOCD_NEUTRAL;
    for (uint8_t i = 0; i < IN_COUNT; ++i) profile.remap[i] = i;
  }
  return value;
}

int main() {
  Config value = validConfig();
  assert(isConfigValid(value));
  for (HidMode mode : {MODE_SWITCH, MODE_RESERVED_2, MODE_PS3}) {
    Config migrated = value;
    migrated.defaultMode = mode;
    migrated.defaultMode = migrateLegacyMode(migrated.defaultMode);
    assert(migrated.defaultMode == MODE_GAMEPAD);
    assert(strcmp(migrated.adminPassword, value.adminPassword) == 0);
    assert(memcmp(migrated.profiles, value.profiles, sizeof(value.profiles)) == 0);
  }
  assert(isSelectableMode(MODE_GAMEPAD));
  assert(isSelectableMode(MODE_DIRECTINPUT));
  assert(isSelectableMode(MODE_KEYBOARD));
  assert(!isSelectableMode(MODE_RESERVED_2));
  assert(!isSelectableMode(MODE_PS3));
  assert(!isSelectableMode(MODE_SWITCH));
  Config erased;
  memset(&erased, 0xFF, sizeof(erased));
  assert(!isConfigValid(erased));
  value.magic = 0;
  assert(!isConfigValid(value));
  value = validConfig();
  for (int mode = 0; mode < 256; ++mode) {
    value.defaultMode = static_cast<HidMode>(mode);
    assert(isConfigValid(value) == (mode < MODE_WEB));
  }
  value = validConfig();
  for (int index = 0; index < 256; ++index) {
    value.activeProfile = static_cast<uint8_t>(index);
    assert(isConfigValid(value) == (index < 3));
  }
  value = validConfig();
  memset(value.adminPassword, 'x', sizeof(value.adminPassword));
  assert(!isConfigValid(value));
  value.adminPassword[23] = '\0';
  assert(isConfigValid(value));
  value.adminPassword[7] = '\0';
  assert(!isConfigValid(value));
  for (int slot = 0; slot < 3; ++slot) {
    value = validConfig();
    memset(value.profiles[slot].name, 'x', sizeof(value.profiles[slot].name));
    assert(!isConfigValid(value));
    value = validConfig();
    for (int hz = 0; hz < 256; ++hz) {
      value.profiles[slot].turboHz = static_cast<uint8_t>(hz);
      assert(isConfigValid(value) == (hz >= 2 && hz <= 30));
    }
    value = validConfig();
    for (int socd = 0; socd < 256; ++socd) {
      value.profiles[slot].socd = static_cast<SocdMode>(socd);
      assert(isConfigValid(value) == (socd <= SOCD_LAST_INPUT));
    }
    value = validConfig();
    value.profiles[slot].turboMask = 0xFFF0;
    assert(isConfigValid(value));
    value.profiles[slot].turboMask |= 1;
    assert(!isConfigValid(value));
    for (int physical = 0; physical < IN_COUNT; ++physical) {
      for (int target = 0; target < 256; ++target) {
        value = validConfig();
        value.profiles[slot].remap[physical] = static_cast<uint8_t>(target);
        bool allowed = target < IN_COUNT &&
            ((physical == IN_SPECIAL) == (target == IN_SPECIAL));
        assert(isConfigValid(value) == allowed);
      }
    }
  }
  puts("Configuration validation tests passed.");
}
