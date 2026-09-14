# BT-GP2350B-Plus-W-CE firmware guide

See the [project README](../README.md) for current support and validation status.

Bluetooth HID controller firmware prototype for the Waveshare RP2350B-Plus-W, based on the requirements in [btpad.md](../btpad.md).

Development order and acceptance gates are in [DEVELOPMENT_PLAN.md](../DEVELOPMENT_PLAN.md); current work is tracked in [TASKS.md](../TASKS.md).

As of 2026-09-14, all eight desktop suites pass, including GP20 boot selection, input routing and OLED bounds. Both builds pass. The OLED/GP20 image was flashed with startup verified; physical display and shortcut checks remain pending. The wiring tester and normal DirectInput were confirmed by the user; Generic HID and reconnect changes still need host testing. See [VALIDATION.md](../VALIDATION.md).

## Hardware Target

- Board: Waveshare RP2350B-Plus-W
- MCU: RP2350B
- Radio: Raspberry Pi Radio Module 2 with Wi-Fi 4 and Bluetooth 5.2
- Firmware stack: Arduino-Pico, because it currently provides RP2350/RP2350W board support plus Bluetooth Classic HID helpers for keyboard and joystick output.

## Compatibility status

Selectable firmware outputs are **Generic HID**, **DirectInput**, and **Keyboard**. The first two use standard Bluetooth Classic HID gamepad reports. Generic HID replaces the misleading XInput name. Native Windows XInput, Switch and PS3 remain separate development targets and cannot currently be selected. The retired console mode has been removed from the firmware; its numeric slot is reserved only to preserve existing EEPROM layout.

Saved defaults for unimplemented/removed modes migrate to Generic HID without resetting passwords or profiles. GP06 selects Generic HID; GP09 selects Keyboard; GP14 selects setup. The former GP07/GP08/GP11 mode shortcuts no longer select placeholder protocols.

Generic HID sends directions through a real eight-direction POV hat; X/Y remain centered. LT and RT drive independent Z and Rz axes without duplicate button 7/8 presses. Their released value is the axis minimum, even with no switch connected; this is not a low GPIO reading. Digital switches only provide released/fully pressed endpoints. DirectInput retains its user-verified legacy X/Y directions and trigger axis/button reports. Guide remains ordinary button 13; native Game Bar/Steam Guide behavior is not implemented.

Outside sync mode, the controller stays connectable and tries saved Bluetooth hosts in turn, at intervals of at least 10 seconds while idle. It avoids starting an attempt while the stack already has an active or pending connection. New pairing still requires GP20 held for three seconds. The pinned core stores bonds in its firmware flash area: reflashing can erase bonds, so pair again after an update before testing reconnection across ordinary power cycles.

For MiSTer, use Generic HID's hat and rerun **Define joystick buttons** after updating the output behavior. Physical MiSTer compatibility and reconnect behavior still require testing. See the [MiSTer controller setup instructions](https://mister-devel.github.io/MkDocs_MiSTer/setup/controller/).

The mapping table below retains the desired labels for future XInput, Switch and PS3 implementations; it is not a claim that those protocols work. See [PROTOCOLS.md](../PROTOCOLS.md).

## GPIO Map

### Live wiring test

Hold GP14 while powering on, connect to **BTPad-Setup**, open **http://192.168.4.1/** and sign in. The **Live wiring test** at the top shows all 19 inputs, including GP14 Special, GP20 Guide/Sync and GP21 Capture. Hold a button or move the joystick: its GPIO badge turns green and reads **Pressed**, both in the tester and mapping table. Release it to clear the highlight.

This reads debounced physical pins before remapping, SOCD and turbo; opposite directions and multiple buttons remain visible together. Setup mode does not run Special/Sync shortcuts, and testing does not save settings. Updates arrive roughly every 100 ms; hold very short taps a little longer. Connection failures clear the highlights and retry; an expired session asks you to sign in again. JavaScript must be enabled.

All inputs are active-low and should wire each button between the GPIO and GND. The sketch enables internal pull-ups.

| GPIO | XInput | Switch | PS3 | DirectInput | Keyboard |
| --- | --- | --- | --- | --- | --- |
| GP02 | UP | UP | UP | UP | Up arrow |
| GP03 | DOWN | DOWN | DOWN | DOWN | Down arrow |
| GP04 | RIGHT | RIGHT | RIGHT | RIGHT | Right arrow |
| GP05 | LEFT | LEFT | LEFT | LEFT | Left arrow |
| GP06 | A | B | Cross | 2 | z |
| GP07 | B | A | Circle | 3 | x |
| GP10 | X | Y | Square | 1 | a |
| GP11 | Y | X | Triangle | 4 | s |
| GP13 | LB | L | L1 | 5 | v |
| GP12 | RB | R | R1 | 6 | f |
| GP09 | LT | ZL | L2 | 7 | c |
| GP08 | RT | ZR | R2 | 8 | d |
| GP17 | Back | Minus | Select | 9 | Backspace |
| GP16 | Start | Plus | Start | 10 | Enter |
| GP18 | LS | LS | L3 | 11 | b |
| GP19 | RS | RS | R3 | 12 | g |
| GP14 | Special | Special | Special | Special | Special |
| GP20 | Guide | Home | PS | 13 | Esc |
| GP21 | - | Capture | - | - | - |

## Boot Mode Selection

Hold one button while powering the board:

| Held GPIO | Boot mode |
| --- | --- |
| GP06 | Generic HID |
| GP09 | Keyboard |
| GP14 | WLAN setup portal |

GP06 selects Generic HID. DirectInput can be selected as the saved default in the web UI; no button held at boot uses that saved default.

## Special Button

GP14 is the special/profile/turbo button.

- Hold Special + Up: next profile
- Hold Special + Down: previous profile
- Hold Special + Left: decrease turbo speed
- Hold Special + Right: increase turbo speed
- Hold Special + any game button: toggle turbo for that physical button

Press Special first, then the shortcut input. One action is allowed per Special hold; priority is Up, Down, Left, Right, then the first held game button in GPIO-map order. All inputs held during Special are suppressed individually until released, including after Special is released. Inputs already sent before Special was pressed cannot be withdrawn. A speed shortcut at the 2/30 Hz limit is consumed without changing another setting. GP20 sync/taps are also suppressed if they belong to a Special gesture.

## Bluetooth sync

Hold GP20 for three seconds to toggle Bluetooth sync on/off in either gamepad or keyboard mode. One hold triggers once; release before toggling again. Sync starts off after boot. Enabling it makes the device discoverable, enables bonding and automatic Just Works confirmation, and disconnects the current HID connection so another host can pair. Disabling it turns off discovery and new bonding/automatic confirmation. Existing bonds are retained for known-host reconnection.

The pairing fix advertises only the selected gamepad or keyboard descriptor, avoiding overflow in Arduino-Pico's fixed service-record buffer. If Windows retained an entry from the earlier firmware, remove that BTPad entry before retrying pairing. USB serial at 115200 baud reports sync, connection, pairing and authentication status once per second (`0xff` means no result yet; `0x00` means success).

A short GP20 press produces its mapped Guide input on release, for one report. A long hold produces no Guide input. GP21 is now an ordinary held Capture input. GP20 sync is inactive in WLAN setup mode. Pairing and reconnect behavior still require testing on the board.

Hold GP20 while plugging in or restarting the board to enter the RP2350 ROM USB flashing mode. It takes priority over other boot shortcuts and runs before display, storage or Bluetooth initialization. Release it once the boot drive appears and copy the UF2. During normal play a GP20 hold only toggles Bluetooth sync. Until firmware containing this shortcut has been installed, use the board's physical BOOTSEL button.

## WLAN Setup Portal

Hold GP14 while powering up. The board starts an access point:

- SSID: `BTPad-Setup`
- Initial Wi-Fi and admin password: `changeme` (existing configurations use their saved password)
- Page: `http://192.168.4.1/`

Sign in using the admin password. Initial setup requires replacing `changeme` before any controller settings can be edited. Choose 8–23 printable ASCII characters and confirm the new password. Password changes sign you out; sign in again with the new password. The Wi-Fi password changes to the saved admin password on the next restart, so reconnect using the new password then.

Sessions expire after 15 minutes or on sign-out/reboot. Passwords are never filled into returned pages or URLs. Forms carry request-verification tokens, and protected pages are not cached. The portal uses HTTP on the device's Wi-Fi network; configuration still stores the password in plaintext flash. Password hashing and a dedicated physical recovery workflow remain storage work. Reflashing a UF2 alone does not necessarily erase the saved password.

The portal currently supports:

- default HID mode
- active profile
- profile name
- per-profile remapping
- per-button turbo enable
- turbo frequency
- SOCD mode

The three profiles are currently shared across modes. Select a profile and click **Load profile** before editing it. **Use this profile** activates the profile when saved; saving another profile otherwise leaves the active profile unchanged. Turbo is limited to the twelve physical buttons A through RS (GP06, GP07, GP10, GP11, GP13, GP12, GP09, GP08, GP17, GP16, GP18, GP19).

Saved configuration is validated before use. Invalid settings reset the complete configuration, including the admin password, to defaults. Portal settings are parsed strictly and validated before saving. Rejected edits and reported commit failures preserve the previous live settings; power-loss recovery of flash remains unverified. Versioned storage and physical recovery remain planned work.

## SOCD Modes

- Neutral: Up+Down and Left+Right cancel to neutral.
- Up priority: Up wins over Down; Left+Right cancel.
- Last input: last vertical direction wins; last horizontal direction wins.

The same cleaned state feeds gamepad axes and keyboard arrows. Last-input history follows logical directions after remapping, on every debounced scan. Simultaneous opposite logical presses in one scan cancel to neutral; releasing one leaves the other active. Releasing the winning direction restores a still-held opposite. Additional physical buttons mapped to an already-held direction do not change its priority. Switching profiles resets direction history.

Turbo gates output after SOCD selection. A held opposite does not regain priority during the winning direction's turbo-off phase.

## Optional I2C Display

The display build enables a 128x64 SSD1306 at address `0x3C`; the default scripted build explicitly disables it.

- SDA: GP00
- SCL: GP01

The original fixed layout is inspired by [GP2040-CE's OLED display](https://gp2040-ce.info/web-configurator/menu-pages/display-configuration/): mode and Bluetooth connection/sync status, profile number/name, a D-pad and eight main buttons that fill when pressed, auxiliary input indicators, turbo rings, turbo rate, SOCD and direction-output mode. It shows physical inputs before remapping/SOCD, including GP20 sync holds. Setup mode shows the access-point name and URL. Updates are limited to one per 100 ms; an absent display at 0x3C is skipped. This is not GP2040-CE's full display configurator, mini-menu, splash or input-history implementation.

## Build

From the workspace root in PowerShell:

```powershell
./build.ps1 -Install               # Install pinned core locally and build without OLED
./build.ps1                        # Subsequent build; no downloads needed
./build.ps1 -Install -WithDisplay  # Install pinned OLED libraries and build with OLED
./build.ps1 -WithDisplay           # Subsequent OLED build
```

Requires Arduino CLI (tested with 1.2.0). The script finds it on PATH or in the standard Arduino IDE installation; use `-ArduinoCli 'C:/path/arduino-cli.exe'` if necessary. Compiler, package cache and libraries stay under `.build/`.

Exact target: `rp2040:rp2040:waveshare_rp2350b_plus_w:arch=arm,freq=150,flash=16777216_0,ipbtstack=ipv4btcble`, core 6.1.0. This selects RP2350B, 150 MHz ARM, 16 MB flash without filesystem, and IPv4 + Bluetooth. Optional libraries: BusIO 1.17.4, GFX 1.12.6, SSD1306 2.5.17. No PSRAM is required.

UF2 output: `.build/headless/btpad_pico.ino.uf2` or `.build/display/btpad_pico.ino.uf2`. The script compiles only; it does not upload firmware. GPIO/radio mapping is provided by the exact board variant in the pinned core.

## Desktop tests

From the workspace root, run `./tests/run.ps1` in PowerShell with Visual Studio C++ build tools installed. It checks configuration bounds/migration, real portal handlers using mocked HTTP/storage/clock/entropy, sync-button timing, and shared input/SOCD behavior. Actual browser/network, flash, Bluetooth, and GPIO behavior require the device checks in [VALIDATION.md](../VALIDATION.md).

Run `node tests/wiring_test.cjs` to check the live tester script's highlights, connection recovery and session-expiry behavior with a simulated browser transport.

## Next work

First verify the flashed Generic HID hat, trigger isolation and automatic reconnect on hardware while preserving working DirectInput. SOCD/shortcut, keyboard and persistence checks remain. Native protocol and storage work is tracked in [TASKS.md](../TASKS.md).
