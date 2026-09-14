#pragma once
#include "BoardInputs.h"

// Original fixed 128x64 layout inspired by GP2040-CE's status/button display.
// Physical inputs are shown so remaps, SOCD and sync holds remain observable.
template<class Screen>
void drawControllerDisplay(Screen &screen, const Config &settings, HidMode mode,
                           uint32_t physical, bool syncing, bool connected) {
  screen.clearDisplay();
  screen.setTextSize(1);
  screen.setTextWrap(false);
  screen.setTextColor(1);
  screen.setCursor(0, 0);
  screen.print(mode == MODE_WEB ? "SETUP" : mode == MODE_KEYBOARD ? "HID-KB" :
               mode == MODE_DIRECTINPUT ? "DINPUT" : "GEN-HID");
  screen.setCursor(80, 0);
  screen.print(mode == MODE_WEB ? "WI-FI" : connected ? "BT:ON" : syncing ? "BT:SYNC" : "BT:WAIT");
  if (mode == MODE_WEB) {
    screen.setCursor(0, 14); screen.print("BTPad-Setup");
    screen.setCursor(0, 26); screen.print("192.168.4.1");
    screen.setCursor(0, 40); screen.print("Sign in for wiring");
    screen.setCursor(0, 50); screen.print("test and settings");
    return;
  }
  const Profile &profile = settings.profiles[settings.activeProfile];
  screen.setCursor(0, 10);
  screen.print("P"); screen.print(settings.activeProfile + 1); screen.print(" ");
  for (uint8_t i = 0; i < 18 && profile.name[i]; ++i) screen.print(profile.name[i]);

  const InputId directions[] = {IN_UP, IN_LEFT, IN_RIGHT, IN_DOWN};
  const int dx[] = {14, 4, 24, 14}, dy[] = {20, 29, 29, 38};
  for (uint8_t i = 0; i < 4; ++i) {
    if (physical & (1u << directions[i])) screen.fillRect(dx[i], dy[i], 9, 9, 1);
    else screen.drawRect(dx[i], dy[i], 9, 9, 1);
  }
  const InputId buttons[] = {IN_X, IN_Y, IN_RB, IN_LB, IN_A, IN_B, IN_RT, IN_LT};
  const char labels[] = {'X','Y','R','L','A','B','T','T'};
  for (uint8_t i = 0; i < 8; ++i) {
    const int x = 54 + (i % 4) * 22, y = i < 4 ? 26 : 40;
    const bool held = (physical & (1u << buttons[i])) != 0;
    if (held) screen.fillCircle(x, y, 6, 1);
    else screen.drawCircle(x, y, 6, 1);
    if (profile.turboMask & (1u << buttons[i])) screen.drawCircle(x, y, 4, held ? 0 : 1);
    screen.setTextColor(held ? 0 : 1);
    screen.setCursor(x - 2, y - 3); screen.print(labels[i]);
  }
  const InputId extras[] = {IN_BACK, IN_START, IN_LS, IN_RS, IN_GUIDE, IN_CAPTURE, IN_SPECIAL};
  const char *extraLabels[] = {"BK", "ST", "LS", "RS", "G", "C", "SP"};
  for (uint8_t i = 0; i < 7; ++i) {
    const bool held = (physical & (1u << extras[i])) != 0;
    if (held) screen.fillRect(i * 18, 48, 16, 8, 1);
    screen.setTextColor(held ? 0 : 1);
    screen.setCursor(i * 18 + 1, 48); screen.print(extraLabels[i]);
  }
  screen.setTextColor(1);
  screen.setCursor(0, 56);
  screen.print("T"); screen.print(profile.turboHz);
  screen.print(profile.socd == SOCD_NEUTRAL ? " SOCD-N " : profile.socd == SOCD_UP_PRIORITY ? " SOCD-U " : " SOCD-L ");
  screen.print(mode == MODE_DIRECTINPUT ? "XY" : mode == MODE_KEYBOARD ? "KEY" : "DP");
}
