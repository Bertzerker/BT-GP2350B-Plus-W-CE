#pragma once
#include "Config.h"

class InputPipeline {
public:
  // Called for every debounced scan, not just transmitted reports, so direction
  // order is retained even when two edges fall between reports.
  bool scan(const bool physical[IN_COUNT], Config &config, bool syncPulse, bool keyboard = false, bool reportTick = true) {
    keyboardMode = keyboard;
    bool eligible[IN_COUNT] = {};
    for (uint8_t i = 0; i < IN_COUNT; ++i) {
      if (!physical[i]) blocked[i] = false;
      eligible[i] = physical[i] && !blocked[i];
    }
    bool changed = false;
    const bool special = physical[IN_SPECIAL];
    const bool capture = physical[IN_CAPTURE];
    if (!special && !capture) comboConsumed = false;
    if (special || capture) {
      if (!comboConsumed) {
        Profile &profile = config.profiles[config.activeProfile];
        if (special) {
          if (eligible[IN_UP] || eligible[IN_DOWN]) {
            comboConsumed = true;
            if (eligible[IN_UP] && !eligible[IN_DOWN] && profile.turboHz < 30) { ++profile.turboHz; changed = true; }
            if (eligible[IN_DOWN] && !eligible[IN_UP] && profile.turboHz > 2) { --profile.turboHz; changed = true; }
          } else {
            for (uint8_t i = IN_SOUTH; i <= IN_RS; ++i) if (eligible[i]) {
              profile.turboMask ^= 1u << i;
              changed = comboConsumed = true;
              break;
            }
          }
        } else if (eligible[IN_LEFT] || eligible[IN_RIGHT]) {
          comboConsumed = true;
          if (eligible[IN_LEFT] != eligible[IN_RIGHT]) {
            uint8_t next = nextProfile(config, config.activeProfile, eligible[IN_RIGHT]);
            changed = next != config.activeProfile;
            config.activeProfile = next;
          }
        } else if (eligible[IN_UP] || eligible[IN_DOWN]) {
          comboConsumed = true;
          if (eligible[IN_UP] != eligible[IN_DOWN]) {
            profile.socd = static_cast<SocdMode>((profile.socd + (eligible[IN_UP] ? 1 : 2)) % 3);
            changed = true;
          }
        }
      }
      // Capture alone is deferred to release so a shortcut never sends Capture.
      for (uint8_t i = 0; i < IN_COUNT; ++i)
        if (physical[i] && (special || comboConsumed)) blocked[i] = true;
    }
    if (captureWasDown && !capture && !captureUsed && !blocked[IN_CAPTURE]) capturePending = true;
    if (special || comboConsumed) capturePending = false;
    bool capturePulse = capturePending;
    if (reportTick) capturePending = false;
    if (capture && !captureWasDown) captureUsed = false;
    if (capture && (comboConsumed || special)) captureUsed = true;
    captureWasDown = capture;
    if (trackedProfile != config.activeProfile) {
      previousDirections = 0;
      verticalWinner = horizontalWinner = IN_COUNT;
      trackedProfile = config.activeProfile;
    }
    accepted = 0;
    for (uint8_t i = 0; i < IN_COUNT; ++i) {
      if (i == IN_SPECIAL) continue;
      bool down = i == IN_GUIDE ? syncPulse : (i == IN_CAPTURE ? capturePulse : physical[i]);
      if (down && !blocked[i] && !physical[IN_SPECIAL]) accepted |= 1u << i;
    }
    const Profile &profile = config.profiles[config.activeProfile];
    uint32_t logical = mapped(profile, true);
    uint32_t rising = (logical & ~previousDirections) & 0xFu;
    updateWinner(rising, IN_UP, IN_DOWN, verticalWinner);
    updateWinner(rising, IN_LEFT, IN_RIGHT, horizontalWinner);
    previousDirections = logical & 0xFu;
    cleanedDirections = previousDirections;
    cleanAxis(IN_UP, IN_DOWN, profile.socd, verticalWinner, true);
    cleanAxis(IN_LEFT, IN_RIGHT, profile.socd, horizontalWinner, false);
    return changed;
  }

  uint32_t report(const Profile &profile, bool turboOn) const {
    // SOCD/history uses ungated held state. Turbo must never make a held
    // opposite regain priority just because the winning input is in its off phase.
    uint32_t gated = mapped(profile, turboOn);
    return (gated & ~0xFu) | (gated & cleanedDirections & 0xFu);
  }
  bool suppressed(InputId id) const { return blocked[id]; }

private:
  bool keyboardMode = false;
  bool blocked[IN_COUNT] = {};
  bool comboConsumed = false;
  bool captureWasDown = false, captureUsed = false, capturePending = false;
  uint8_t trackedProfile = 0xff;
  InputId verticalWinner = IN_COUNT, horizontalWinner = IN_COUNT;
  uint32_t accepted = 0, previousDirections = 0, cleanedDirections = 0;

  uint32_t mapped(const Profile &profile, bool turboOn) const {
    uint32_t result = 0;
    for (uint8_t i = 0; i < IN_COUNT; ++i) {
      if ((accepted & (1u << i)) && (turboOn || !(profile.turboMask & (1u << i)))) {
        result |= 1u << (keyboardMode ? i : profile.remap[i]);
      }
    }
    return result;
  }
  static void updateWinner(uint32_t rising, InputId a, InputId b, InputId &winner) {
    bool first = (rising & (1u << a)) != 0, second = (rising & (1u << b)) != 0;
    if (first && second) winner = IN_COUNT; // Same-scan ties are neutral.
    else if (first) winner = a;
    else if (second) winner = b;
  }
  void cleanAxis(InputId a, InputId b, SocdMode mode, InputId winner, bool vertical) {
    uint32_t both = (1u << a) | (1u << b);
    if ((cleanedDirections & both) != both) return;
    cleanedDirections &= ~both;
    if (mode == SOCD_UP_PRIORITY && vertical) cleanedDirections |= 1u << IN_UP;
    else if (mode == SOCD_LAST_INPUT && winner != IN_COUNT) cleanedDirections |= 1u << winner;
  }
};
