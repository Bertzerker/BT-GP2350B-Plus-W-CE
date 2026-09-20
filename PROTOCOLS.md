# Output protocol status and next implementation

Updated 2026-09-09 after physical tests and user feedback.

| Output | Current status | Acceptance requirement |
| --- | --- | --- |
| Generic HID / DirectInput | DirectInput user-confirmed working. Generic HID now sends a real hat and trigger axes without duplicate button bits; saved-host reconnect added | Verify updated Generic HID on Windows/MiSTer and reconnect across ordinary power cycles; DirectInput face numbering now follows btpad.md |
| Keyboard | Implemented; shared SOCD logic tested on desktop | Pair and verify arrows, remaps, rollover and releases on hardware |
| Windows XInput over Bluetooth | User confirmed latest build working; Windows driver healthy and XInput API enumeration observed | Windows XInput API detects device, independent LT/RT work together, native Guide reaches Game Bar/Steam with host settings enabled |
| PS3 | Experimental USB DS3 reports/features; Bluetooth not implemented | Real console pairing, PS button, input/feature/output reports, reconnect |
| Switch | Not implemented | Native controller pairing, commands/reports, Home/Capture and reconnect |

PS4 is outside project scope at the user's request. Its old stored mode ID is reserved and migrates to Generic HID; there is no selectable mode, boot shortcut or button-label set for it.

## Why the previous XInput/PS3 modes failed

Those modes only changed labels and device names on the generic joystick transport. The Windows host successfully treating that device as DirectInput did not establish XInput compatibility. Guide was an ordinary button, and triggers were originally only button bits. The latest generic implementation adds independent Z/Rz trigger endpoints without claiming to implement XInput.

Microsoft documents separate XInput and HID/DirectInput interfaces and their mappings. Its XUSB-to-DirectInput mapping can combine trigger input, so the legacy Windows joystick panel alone is not an adequate native-XInput acceptance test. See [DirectInput and XUSB Devices](https://learn.microsoft.com/en-us/windows/win32/xinput/directinput-and-xusb-devices).

## Native Windows XInput path

Investigate a dedicated BLE HID implementation using the Xbox-compatible report family, device information and host interactions. [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) implements Xbox-style BLE gamepad devices and documents Windows support. It uses NimBLE on ESP32; this is a reference, not a drop-in dependency for this RP2350/BTstack project. Review its license, pin a revision, and inspect descriptor/report/host-configuration sources before adapting anything.

Build a report encoder with independent trigger fields, hat, face/shoulder buttons and Guide. Retain the tested input pipeline. Add a separate BLE transport and sync implementation rather than routing Xbox reports through JoystickBT. Windows device identity and XInputGetState enumeration are now verified; continue testing Guide and reconnect behavior. Driver identity, descriptor shape and bonding must be validated together; copying a device name is insufficient. No claim of Xbox console support follows from Windows Bluetooth XInput support.

GPIO triggers are digital: native trigger reports should expose released/full endpoints. Intermediate analog pressure needs different input hardware.

## Native PS3 path

Use a genuine device-emulation reference rather than a library that only connects to existing controllers. [RosettaPad](https://github.com/ihasTaco/RosettaPad) describes controller emulation for a PS3 and the relevant feature-report exchanges, including host-address pairing. It targets a Linux Raspberry Pi environment, so its USB/Bluetooth integration needs a separate RP2350 design.

Implement and test USB pairing/host-address storage, PS3 HID descriptors and feature reports, Bluetooth input/output handling and PS-button behavior. Start with a USB pairing transcript from the user's console, then test Bluetooth reconnect. Keep the mode unavailable until the console accepts it.

## Firmware migration

Stored mode 0 is now named Generic HID; modes 1 and 2 (unimplemented/retired choices) migrate to 0 after validating the rest of the saved configuration. Mode 3 now selects experimental PS3 USB; modes 4 (DirectInput) and 5 (Keyboard) remain valid. No struct fields, profile data or password storage offsets change. Portal saves reject unavailable mode IDs. GP06 selects Generic HID, GP09 Keyboard, GP14 WLAN; other native-mode boot shortcuts are disabled until implemented.

## Experimental Xbox BLE implementation — 2026-09-15

The new BTstack ATT service advertises BTPad-Xbox and exposes encrypted HID reports, report references, HID information and device information. The 16-byte input report has four centered stick axes, separate 10-bit digital trigger endpoints, an eight-direction hat, face/system buttons, Guide and Capture. Output rumble reports are accepted but no motors are fitted. The battery service currently reports a fixed 100% placeholder, not a measurement. The Xbox-family VID/PID is used for experimental Windows driver matching; this is not Microsoft-certified hardware.

The descriptor is adapted from [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID/tree/06d93eab499181afaa3e26f96ecee67233c01303), pinned at commit `06d93eab499181afaa3e26f96ecee67233c01303`. Its MIT notice is retained in `third_party/ESP32-BLE-CompositeHID-LICENSE.txt`. This is a BTstack port of the descriptor/report family, not the ESP32 transport.

Mode 7 selects experimental Xbox BLE; mode 6 remains setup-only. Existing Config layout and logical IDs remain unchanged. Keyboard maps use a versioned checksummed extension and fall back to defaults if missing/corrupt. Shared gamepad profiles remain independent of protocol.

Acceptance still requires a physical Windows XInputGetState result, independent simultaneous triggers, all hat directions, Guide behavior, pairing and repeated reconnect without sync. Classic reconnect now closes pairing on successful HID connection and clears established ACL links stalled without HID. BLE advertises outside sync for saved-host reconnection but rejects new pairing without GP20. No hardware claim is made from encoder tests.

## Host failures and revised test firmware — 2026-09-20

User screenshots establish Windows binding to xinputhid.inf for VID 045E / PID 0B13 / revision 0509, then Code 10, Kernel-PnP event 411 and NTSTATUS 0xC00000E5. MiSTer reports pairing followed immediately by disconnect. This supports investigating shared firmware behavior, but does not identify a proven root cause. Steam's filter is present in the Windows screenshot; it is not established as causal.

The revision 050A test build adds a Generic Attribute service, encrypted dynamically-read Report Map (with offset/length logs), actual stack encryption-state checks, pairing/subscription/disconnect logs and recovery for links that never reach HID-ready. Known bonds can reconnect outside sync; new pairing still needs GP20. Re-pair after the database change. Capture a complete serial trace on both hosts before claiming a fix.

PS3 USB uses a descriptor and feature byte arrays adapted from [GP2040-CE commit 4ee15e7](https://github.com/OpenStickCommunity/GP2040-CE/tree/4ee15e728f08077b6044e8297f30c69d5182f222). MIT attribution is retained in third_party/GP2040-CE-LICENSE.txt. The report-ID global item is moved to descriptor offset 6 for Arduino-Pico's HID registration; the report layout is unchanged. Feature copies are bounded by both buffer length and source size. PS3 USB acceptance remains pending. This does not implement PS3 Bluetooth pairing or transport.

### Windows result after flashing

The user confirms Xbox BLE works and stays connected. Windows read-only device inspection shows BTPad-Xbox and Bluetooth LE XINPUT compatible input device both OK. XInputGetState succeeds in slot 0. A 55-second serial observation remained connected with sync off. Because another Xbox device is also installed, the API slot alone is not a unique identity proof; the matching healthy BTPad BLE entry and user test corroborate it. Trigger endpoints, Guide, MiSTer and repeated power-cycle reconnect still need explicit acceptance. No single root cause is proven because the revised build changes several startup details together.
