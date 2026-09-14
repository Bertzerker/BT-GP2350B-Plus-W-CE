#pragma once

// Run before storage, display and Bluetooth startup. This invokes the RP2350
// ROM USB loader; holding the button during play cannot enter flash mode.
inline void checkFirmwareBoot() {
  pinMode(20, INPUT_PULLUP);
  delay(10);
  if (digitalRead(20) == LOW) {
    delay(30);
    if (digitalRead(20) == LOW) reset_usb_boot(0, 0);
  }
}
