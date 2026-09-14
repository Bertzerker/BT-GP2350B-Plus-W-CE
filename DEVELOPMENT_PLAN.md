# BTPad development plan

Updated: 2026-09-09. Requirements: [btpad.md](btpad.md). Execution queue: [TASKS.md](TASKS.md).

## Outcome and scope

Build reliable wireless gamepad and keyboard firmware for the Waveshare RP2350B-Plus-W with the specified GPIO wiring, configurable profiles, SOCD, turbo, and optional display. Ship a tested generic Bluetooth HID gamepad and keyboard baseline first. Treat native Switch, PS3, and Xbox compatibility as separate protocol projects with their own host acceptance tests.

The current sketch is a prototype, not a verified release. Exact-target compilation is now scripted; see [VALIDATION.md](VALIDATION.md) for build evidence. Physical pairing tests remain pending. Mode labels currently share the same generic joystick transport; renaming that transport does not implement XInput or console compatibility.

## Decisions

- 2026-09-14: preserve user-confirmed DirectInput behavior. Fix Generic HID hat/trigger duplication and implement outgoing saved-host reconnect before further native protocol work. Reconnection must not enable new pairing or overlap an existing Bluetooth connection attempt.

- Preserve the requested GPIO map and active-low wiring.
- GP06 selects Generic HID, GP09 Keyboard, and GP14 WLAN. Unimplemented native-mode shortcuts are disabled until verified; saved legacy defaults migrate without losing passwords/profiles.
- Keep Wi-Fi configuration and Bluetooth play as separate boot modes.
- Prioritize the web wiring tester for the user's button rewiring: show every physical GPIO before remapping/SOCD/turbo, including Special and Sync, with live highlights and no configuration writes. Resume native transport work after this addition.
- Reserve GP14 for local control. It must not be a normal remap destination.
- GP21 held for three seconds toggles Bluetooth sync/discovery (user update 2026-09-09). Short presses produce mapped Capture on release; long holds suppress that host input. Keep existing bonds and known-host reconnect available. Hardware pairing checks are SYNC-01.
- Keep the existing three shared profiles only during baseline stabilization. The requested end state is multiple independent profiles per output mode.
- Keep the existing EEPROM layout for the first validation fix. Versioning, migration, integrity checks, and write recovery belong in the storage milestone.
- Do not select a merely similar board definition. Verify the exact board/radio variant before compiling or flashing.

## Milestones and acceptance gates

### 1. Reproducible baseline and configuration validation

Record the exact board definition, Arduino-Pico version, Bluetooth stack option, and display dependencies. Produce a repeatable compile command and record firmware size. Extract pure logic only where it enables useful tests; avoid a broad rewrite.

Validate persisted mode, profile index, strings, remaps, turbo range, and SOCD before use. Reject invalid edits without changing live settings. Gate: desktop validation tests pass, exact-target firmware compiles, and erased or corrupt configuration recovers on hardware. Desktop tests alone do not satisfy this milestone.

### 2. Correct input and generic HID behavior

Use one pipeline: scan/debounce -> special-combo consumption -> remapping -> logical direction history/SOCD -> turbo -> transport report. Specify how remapped directions, simultaneous opposites, release/reactivation, and profile changes behave before implementing tests. Apply SOCD to keyboard output as well as gamepad output. Keep control chords out of host reports until their buttons are released.

Check joystick API ranges, report batching, button numbering (including the requested DirectInput 2/3/1 ordering), keyboard rollover, and release behavior. Gate: automated edge-case tests plus recorded PC pairing, reconnect, sustained input, and stuck-key checks. Measure scan-to-report timing; a 4 ms loop interval is not measured wireless latency.

### 3. Usable setup portal and durable per-mode profiles

Fix password disclosure, enforce initial password replacement, protect reads and writes, remove URL credentials, escape user-provided HTML, and add request forgery protection. Specify credential recovery and the relation between AP and admin passwords. Separate profile selection from saving an edited profile so selecting another profile cannot overwrite it with the old form contents.

Introduce versioned configuration with integrity checks, explicit migration/reset behavior, checked writes, and wear-conscious persistence. Add independent profile banks per mode, schema validation, and import/export. Gate: first-run/login/save/reload/logout tests, unauthorized request rejection, profile isolation, and interrupted-save recovery on hardware.

### 4. Device usability and hardware qualification

Add pairing reset and reconnect controls. Rate-limit display updates by elapsed time instead of a modulo window, and keep display work from repeatedly delaying input reports. Show effective profile, output transport, inputs, and enabled turbo. Gate: tests with and without display libraries, hardware power-cycle/reconnect checks, and latency/power measurements under a documented supply setup. Battery hardware remains unspecified.

### 5. Native protocol feasibility, one host at a time

Research each target's pairing, descriptors, input/output reports, feature commands, and authentication requirements. Choose an implementation order from demonstrated feasibility and available test hardware. Provide a support matrix distinguishing generic HID with themed labels from verified native operation. Gate for each mode: connect to its actual target, verify all controls and host commands, and survive reconnect and power cycles. Do not advertise a native mode before this gate.

## Verification and references

- Desktop suite: `./tests/run.ps1` (Visual Studio C++ tools). Tests use the actual configuration validator, portal route handlers and sync-button state machine. HTTP, storage, time and entropy are mocked for route tests; real network/EEPROM behavior is not covered.
- Arduino CLI 1.2.0 and project-local Arduino-Pico 6.1.0 are configured by `build.ps1`. Default output disables OLED; `-WithDisplay` requires pinned display libraries.
- [Arduino-Pico Bluetooth documentation](https://arduino-pico.readthedocs.io/en/latest/bluetooth.html) confirms Classic HID helpers and the required Bluetooth stack setting; it does not establish console compatibility.
- [Waveshare target reference](https://www.waveshare.com/wiki/RP2350B-Plus-W) and the [pinned board variant](https://github.com/earlephilhower/arduino-pico/blob/6.1.0/variants/waveshare_rp2350b_plus_w/pins_arduino.h) identify RP2350B, RM2 radio, and GPIO36–39 radio wiring. Build uses the exact variant, not a substitute Pico board.

Keep release evidence with exact firmware/core versions, host OS or console version, board revision, wiring, observed results, and unresolved defects.
