# BT-GP2350B-Plus-W-CE Bluetooth Controller Firmware

Bluetooth controller firmware for the **Waveshare RP2350B-Plus-W**, built with Arduino-Pico. It supports wired buttons and a digital joystick, Bluetooth gamepad/keyboard output, a Wi-Fi configuration page with a live wiring tester, and an optional SSD1306 OLED.

**Status: working development prototype, updated 2026-09-14.** The OLED firmware with the new layout and GP20 sync/boot selector has been flashed and its startup verified. Physical display and shortcut checks remain pending. Generic HID and automatic reconnect changes also still need host testing.

## Current support

| Feature | State |
| --- | --- |
| DirectInput | User confirmed normal operation; its existing output behavior is preserved. |
| Generic HID | Eight-direction POV hat and independent Z/Rz triggers without duplicate trigger buttons. Desktop tests pass; latest Windows/MiSTer checks pending. |
| Keyboard | Implemented with shared input processing; full hardware validation pending. |
| Bluetooth sync | Hold GP20 for 3 seconds to toggle pairing/discovery. Basic pairing has been confirmed. |
| Automatic reconnect | Saved-host attempts added, with at least 10 seconds between idle attempts. Desktop tests pass; power-cycle reconnect unverified. |
| Web setup | Password/default-mode changes smoke-tested; live GPIO wiring tester confirmed working by the user. |
| Profiles and input processing | Three shared profiles, remapping, turbo and SOCD implemented with desktop tests. Broader persistence/input hardware checks pending. |
| Optional OLED | No-OLED and OLED firmware builds pass; OLED hardware validation pending. |

Generic HID and DirectInput use **Bluetooth Classic HID**. Neither implements native Windows XInput. Native XInput, PS3 and Switch are planned and are unavailable in the mode selector. PS4 support is removed from scope. Guide is currently an ordinary button and does not provide native Game Bar/Steam Guide behavior.

The inputs are digital switches, so trigger axes have only released and fully pressed values. A trigger axis resting at its minimum is expected, including when its switch is disconnected. DirectInput retains its earlier axis/button trigger output; the duplicate-button correction applies to Generic HID.

## Getting started

1. Wire inputs between their GPIO and GND; internal pull-ups are enabled. See the [GPIO map and controls](btpad_pico/README.md).
2. Hold **GP14 at power-on** to enter setup. Connect to **BTPad-Setup** and open **http://192.168.4.1/**.
3. On a fresh configuration, the Wi-Fi and admin password is **`changeme`**. Replace it when prompted. An existing saved password remains in use; Wi-Fi adopts a changed password on the next restart.
4. Use **Live wiring test** to see GPIO labels light up while inputs are held. Choose the default mode and restart with no buttons held.
5. Hold **GP20 for 3 seconds** to enable Bluetooth sync and pair from the host. Release before using another long hold to toggle sync off.

Boot overrides: **GP06 = Generic HID**, **GP09 = Keyboard**, **GP14 = setup**. DirectInput is selected through the saved default. Wi-Fi setup and Bluetooth play run in separate boot modes.

**Hold GP20 at power-on to enter USB firmware flashing mode.** This takes priority over all boot overrides. In normal play, GP20 short taps send Guide and a three-second hold toggles sync; GP21 is an ordinary Capture input. The new boot shortcut becomes available only after installing this update.

With the display fitted, use the **`-WithDisplay`** build. Its GP2040-CE-inspired screen shows mode, Bluetooth status, profile, live D-pad/buttons, turbo rings/rate and SOCD; setup mode shows the Wi-Fi name and URL. The expected display remains a 128×64 SSD1306 at `0x3C` on GP00/GP01. Hardware display and GP20 boot/sync checks remain pending.

After the latest update, test Generic HID's POV directions and trigger isolation, then pair once and power-cycle without sync to test reconnect. Redefine the controller inputs on MiSTer after changing direction-report behavior.

## Build and test

Requires Arduino CLI (tested with 1.2.0). The build script installs pinned dependencies into the ignored `.build/` directory:

```powershell
./build.ps1 -Install                 # Install dependencies and build without OLED
./build.ps1                          # Subsequent no-OLED build
./build.ps1 -Install -WithDisplay     # Install OLED dependencies and build with OLED
./build.ps1 -WithDisplay             # Subsequent OLED build
./tests/run.ps1                      # Eight C++ suites; Visual Studio C++ tools required
node tests/wiring_test.cjs           # Wiring-page script tests; Node.js required
```

Pinned target: Arduino-Pico **6.1.0**, `waveshare_rp2350b_plus_w`, ARM at 150 MHz, 16 MB flash, IPv4 + Bluetooth. Optional display: SSD1306 128×64 at `0x3C`, SDA GP00 / SCL GP01. See the [firmware guide](btpad_pico/README.md) for the full board identifier and dependency versions.

Outputs are `.build/headless/btpad_pico.ino.uf2` and `.build/display/btpad_pico.ino.uf2`. Enter BOOTSEL and copy the appropriate UF2 to the RP2350 boot drive. The build script does not flash automatically.

All eight C++ suites and the wiring-script tests pass for the current implementation. Both display and headless builds pass; see [VALIDATION.md](VALIDATION.md) for exact build and flash records. The OLED image was flashed successfully and returned live serial diagnostics; physical OLED/GP20 behavior still requires user verification.

## Known limitations and next work

- Verify the latest Generic HID behavior on Windows/MiSTer and automatic reconnect across power cycles and host availability changes.
- The pinned core stores Bluetooth bonds inside the firmware flash area; reflashing can require pairing again. Saved configuration/password storage is separate and is not necessarily erased by a UF2 update.
- Profiles are shared across modes. Versioned storage, power-loss recovery, per-mode profile banks and import/export remain planned.
- The setup portal uses HTTP and stores its password in plaintext flash. It provides timed sessions and request verification; a dedicated physical password-recovery workflow remains planned.
- Native XInput and console transports require separate protocol implementations and hardware acceptance tests.

## Project contents

- [Firmware and detailed usage](btpad_pico/README.md)
- [Development plan](DEVELOPMENT_PLAN.md) and [task list](TASKS.md)
- [Protocol status and research](PROTOCOLS.md)
- [Validation record](VALIDATION.md)
- [Original requirements](btpad.md), including future targets rather than only implemented features

Source, documentation, tests and build scripts are versioned. Downloaded toolchains, build logs, test executables and generated firmware are excluded from Git.
