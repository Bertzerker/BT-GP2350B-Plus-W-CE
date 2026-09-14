#pragma once
#include <HID_Bluetooth.h>
#include <sdkoverride/tusb_gamepad16.h>

// Both helper libraries are linked, so JoystickBT sends report 3, while
// KeyboardBT sends reports 1 and 2. Advertise only the selected mode.
static const uint8_t gamepadDescriptor[] = {
  TUD_HID_REPORT_DESC_GAMEPAD16(HID_REPORT_ID(3))
};
static const uint8_t keyboardDescriptor[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(2))
};
static const char *const bluetoothNames[] = {
  "BTPad-Gamepad", "", "", "",
  "BTPad-DirectInput", "BTPad-Keyboard"
};

// Arduino-Pico 6.1.0 has a 300-byte SDP record buffer. BTstack's HID record
// adds 183 fixed bytes plus two 2-byte data headers, name and descriptor.
// The helpers combine all linked descriptors, overflowing that buffer here.
static_assert(187 + sizeof("BTPad-DirectInput") - 1 + sizeof(gamepadDescriptor) <= 300,
              "Gamepad SDP record exceeds the pinned core buffer");
static_assert(187 + sizeof("BTPad-Keyboard") - 1 + sizeof(keyboardDescriptor) <= 300,
              "Keyboard SDP record exceeds the pinned core buffer");
