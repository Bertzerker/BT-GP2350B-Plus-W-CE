# Output protocol status and next implementation

Updated 2026-09-09 after physical tests and user feedback.

| Output | Current status | Acceptance requirement |
| --- | --- | --- |
| Generic HID / DirectInput | DirectInput user-confirmed working. Generic HID now sends a real hat and trigger axes without duplicate button bits; saved-host reconnect added | Verify updated Generic HID on Windows/MiSTer and reconnect across ordinary power cycles; DirectInput behavior preserved |
| Keyboard | Implemented; shared SOCD logic tested on desktop | Pair and verify arrows, remaps, rollover and releases on hardware |
| Windows XInput over Bluetooth | Not implemented; old XInput label removed | Windows XInput API detects device, independent LT/RT work together, native Guide reaches Game Bar/Steam with host settings enabled |
| PS3 | Not implemented; user confirmed placeholder fails | Real console pairing, PS button, input/feature/output reports, reconnect |
| Switch | Not implemented | Native controller pairing, commands/reports, Home/Capture and reconnect |

PS4 is outside project scope at the user's request. Its old stored mode ID is reserved and migrates to Generic HID; there is no selectable mode, boot shortcut or button-label set for it.

## Why the previous XInput/PS3 modes failed

Those modes only changed labels and device names on the generic joystick transport. The Windows host successfully treating that device as DirectInput did not establish XInput compatibility. Guide was an ordinary button, and triggers were originally only button bits. The latest generic implementation adds independent Z/Rz trigger endpoints without claiming to implement XInput.

Microsoft documents separate XInput and HID/DirectInput interfaces and their mappings. Its XUSB-to-DirectInput mapping can combine trigger input, so the legacy Windows joystick panel alone is not an adequate native-XInput acceptance test. See [DirectInput and XUSB Devices](https://learn.microsoft.com/en-us/windows/win32/xinput/directinput-and-xusb-devices).

## Native Windows XInput path

Investigate a dedicated BLE HID implementation using the Xbox-compatible report family, device information and host interactions. [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) implements Xbox-style BLE gamepad devices and documents Windows support. It uses NimBLE on ESP32; this is a reference, not a drop-in dependency for this RP2350/BTstack project. Review its license, pin a revision, and inspect descriptor/report/host-configuration sources before adapting anything.

Build a report encoder with independent trigger fields, hat, face/shoulder buttons and Guide. Retain the tested input pipeline. Add a separate BLE transport and sync implementation rather than routing Xbox reports through JoystickBT. Verify any necessary device-identity matching against the Windows driver on the test host, then check XInputGetState and Guide behavior. Driver identity, descriptor shape and bonding must be validated together; copying a device name is insufficient. No claim of Xbox console support follows from Windows Bluetooth XInput support.

GPIO triggers are digital: native trigger reports should expose released/full endpoints. Intermediate analog pressure needs different input hardware.

## Native PS3 path

Use a genuine device-emulation reference rather than a library that only connects to existing controllers. [RosettaPad](https://github.com/ihasTaco/RosettaPad) describes controller emulation for a PS3 and the relevant feature-report exchanges, including host-address pairing. It targets a Linux Raspberry Pi environment, so its USB/Bluetooth integration needs a separate RP2350 design.

Implement and test USB pairing/host-address storage, PS3 HID descriptors and feature reports, Bluetooth input/output handling and PS-button behavior. Start with a USB pairing transcript from the user's console, then test Bluetooth reconnect. Keep the mode unavailable until the console accepts it.

## Firmware migration

Stored mode 0 is now named Generic HID; modes 1, 2 and 3 (unimplemented/retired choices) migrate to 0 after validating the rest of the saved configuration. Modes 4 (DirectInput) and 5 (Keyboard) remain valid. No struct fields, profile data or password storage offsets change. Portal saves reject unavailable mode IDs. GP06 selects Generic HID, GP09 Keyboard, GP14 WLAN; other native-mode boot shortcuts are disabled until implemented.
