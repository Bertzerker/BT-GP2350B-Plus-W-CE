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

Selectable firmware outputs are **Generic HID**, **DirectInput**, **Keyboard**, **PS3 USB (experimental)**, and **Xbox BLE (experimental)**. The first two use standard Bluetooth Classic HID gamepad reports. Generic HID replaces the misleading XInput name. Xbox BLE uses a separate transport; Windows driver health and XInput API enumeration are now verified. Switch and PS3 Bluetooth remain unavailable; PS3 USB is experimental. The retired console mode has been removed from the firmware; its numeric slot is reserved only to preserve existing EEPROM layout.

Saved defaults for unimplemented/removed modes migrate to Generic HID without resetting passwords or profiles. GP06 selects Generic HID; GP09 selects Keyboard; GP14 selects setup. GP08 selects PS3 USB, GP10 Xbox BLE, and GP11 DirectInput; GP07 is reserved for Switch.

Generic HID sends directions through a real eight-direction POV hat; X/Y remain centered. LT and RT drive independent Z and Rz axes without duplicate button 7/8 presses. Their released value is the axis minimum, even with no switch connected; this is not a low GPIO reading. Digital switches only provide released/fully pressed endpoints. DirectInput retains its user-verified legacy X/Y directions and trigger axis/button reports. Guide remains ordinary button 13; native Game Bar/Steam Guide behavior is not implemented.

Outside sync mode, the controller stays connectable and tries saved Bluetooth hosts in turn, at intervals of at least 10 seconds while idle. It avoids starting an attempt while the stack already has an active or pending connection. New pairing still requires GP20 held for three seconds. The pinned core stores bonds in its firmware flash area: reflashing can erase bonds, so pair again after an update before testing reconnection across ordinary power cycles.

For MiSTer, use Generic HID's hat and rerun **Define joystick buttons** after updating the output behavior. Physical MiSTer compatibility and reconnect behavior still require testing. See the [MiSTer controller setup instructions](https://mister-devel.github.io/MkDocs_MiSTer/setup/controller/).

The mapping table below retains the desired labels for future XInput, Switch and PS3 implementations; it is not a claim that those protocols work. See [PROTOCOLS.md](../PROTOCOLS.md).

## GPIO Map

### Live wiring test

Hold GP14 while powering on, connect to **BTPad-Setup**, open **http://192.168.4.1/** and sign in. In **Profile mapper**, hold a button or move the joystick: its GPIO badge turns green and reads **Pressed**. Release it to clear the highlight. Each of the 19 inputs appears once; GP14 Special has a fixed read-only row. There is no separate wiring-test box.

The page uses the artwork in `style/` for its favicon and background, with smaller embedded derivatives. Forms and mapper rows fit narrow mobile screens, with 16px form text and touch-sized controls. See [artwork build and preview instructions](../style/README.md).

This reads debounced physical pins before remapping, SOCD and turbo; opposite directions and multiple buttons remain visible together. Setup mode does not run Special/Sync shortcuts, and testing does not save settings. Updates arrive roughly every 100 ms; hold very short taps a little longer. Connection failures clear the highlights and retry; an expired session asks you to sign in again. JavaScript must be enabled.

All inputs are active-low and should wire each button between the GPIO and GND. The sketch enables internal pull-ups.

| GPIO | Maps to | XInput | Switch | PS3 | DirectInput | Keyboard |
| --- | --- | --- | --- | --- | --- | --- |
| GP02 | UP | UP | UP | UP | UP | Up arrow |
| GP03 | DOWN | DOWN | DOWN | DOWN | DOWN | Down arrow |
| GP04 | RIGHT | RIGHT | RIGHT | RIGHT | RIGHT | Right arrow |
| GP05 | LEFT | LEFT | LEFT | LEFT | LEFT | Left arrow |
| GP06 | South | A | B | Cross | 2 | z |
| GP07 | East |B | A | Circle | 3 | x |
| GP10 | West | X | Y | Square | 1 | a |
| GP11 | North |Y | X | Triangle | 4 | s |
| GP13 | L1 |LB | L | L1 | 5 | v |
| GP12 | R1 | RB | R | R1 | 6 | f |
| GP09 | L2 |LT | ZL | L2 | 7 | c |
| GP08 | R2 |RT | ZR | R2 | 8 | d |
| GP17 | Select | Back | Minus | Select | 9 | Backspace |
| GP16 | Start |Start | Plus | Start | 10 | Enter |
| GP18 | LS | LS | LS | L3 | 11 | b |
| GP19 | RS | RS | RS | R3 | 12 | g |
| GP14 | Special |Special | Special | Special | Special | Special |
| GP20 | Guide | Guide | Home | PS | 13 | Esc |
| GP21 | Capture | - | Capture | - | 14 | TAB |

## Boot Mode Selection

Hold one button while powering the board:

| Held GPIO | Boot mode |
| --- | --- |
| GP06 | Generic HID |
| GP07 | Reserved for future Switch; uses saved default today |
| GP08 | PS3 USB (experimental) |
| GP09 | Keyboard |
| GP10 | Xbox BLE |
| GP11 | DirectInput |
| GP12 | Xbox Classic Bluetooth (experimental) |
| GP14 | WLAN setup portal |
| GP20 | USB firmware flashing |

With no button held, the saved default is used. GP20 flashing has highest priority, followed by GP14 setup; hold only one mode button.

## Runtime shortcuts

- Special GP14 + Home/Guide GP20: reconnect using the existing identity and pairing (USB reconnect in PS3 mode). The host decides the player slot and may keep the same one. Release both buttons to repeat; the chord does not send Guide or toggle pairing. Classic Bluetooth may take about 10 seconds to reconnect.

- Capture GP21 + Left/Right: previous/next existing profile.
- Capture GP21 + Up/Down: cycle SOCD forward/backward.
- Special GP14 + Up/Down: increase/decrease turbo speed, 2–30 Hz.
- Special + a game button: toggle its turbo.

Press the modifier first. Each modifier hold performs one action; release before the next shortcut. All inputs participating in a shortcut stay suppressed until individually released. Capture alone sends on release. Already-transmitted inputs cannot be withdrawn if a modifier is pressed later.

## Bluetooth sync

Hold GP20 for three seconds to toggle Bluetooth sync on/off in either gamepad or keyboard mode. One hold triggers once; release before toggling again. Sync starts off after boot. Enabling it makes the device discoverable, enables bonding and automatic Just Works confirmation, and disconnects the current HID connection so another host can pair. Disabling it turns off discovery and new bonding/automatic confirmation. Existing bonds are retained for known-host reconnection.

The pairing fix advertises only the selected gamepad or keyboard descriptor, avoiding overflow in Arduino-Pico's fixed service-record buffer. If Windows retained an entry from the earlier firmware, remove that BTPad entry before retrying pairing. USB serial at 115200 baud reports sync, connection, pairing and authentication status once per second (`0xff` means no result yet; `0x00` means success).

A short GP20 press produces its mapped Guide input on release, for one report. A long hold produces no Guide input. GP21 sends Capture on release unless used in a profile/SOCD shortcut. GP20 sync is inactive in WLAN setup mode. Pairing and reconnect behavior still require testing on the board.

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

The three profiles are currently shared across modes. Select a profile and click **Load profile** before editing it. **Use this profile** activates the profile when saved; saving another profile otherwise leaves the active profile unchanged. Turbo is limited to the twelve physical buttons South through RS (GP06, GP07, GP10, GP11, GP13, GP12, GP09, GP08, GP17, GP16, GP18, GP19).

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

## Shared profiles and keyboard mapping

The Profile mapper uses neutral btpad.md names (South/East/West/North, L1/R1/L2/R2 and system buttons), independent of selected protocol. DirectInput face numbering is West=1, South=2, East=3, North=4. Switch is still unimplemented; shared profiles are ready for future protocol adapters.

Expand **Keyboard mapper for this profile** to map each physical GPIO to a standard keyboard usage, including modifiers, punctuation, navigation, keypad, F1–F24 and international keys. Keyboard mappings do not pass through gamepad remaps. Special stays reserved; GP20 is a short-release key and long-hold sync control. Six ordinary keys plus eight modifiers can be held simultaneously. More ordinary keys report rollover; host layout determines characters. Settings are saved in a checksummed extension without changing existing profile/password offsets.

To test Xbox BLE, select it as default in setup, reboot, hold GP20 for three seconds and pair **BTPad-Xbox**. Pairing automatically closes after connection. Remove stale host pairing records and pair again after reflashing if necessary. Run `./tools/test-xinput.ps1` on Windows: enumeration in the joystick control panel alone is not proof of XInput. Verify Guide separately with Game Bar/Steam settings enabled. Neither Xbox-console support nor native Switch support is implemented.

## Current controls and PS3 test mode — 2026-09-20

Current shortcut summary:

- Capture GP21 + Left/Right: previous/next non-deleted profile.
- Capture GP21 + Up/Down: cycle SOCD forward/backward.
- Special GP14 + Up/Down: increase/decrease turbo speed (2–30 Hz).
- Special + a game button: toggle turbo. One action per modifier hold; release to rearm.
- Capture alone sends on release; shortcut chords do not send Capture or their directions.

Create/delete profile controls manage the three existing storage slots. New profiles start with default gamepad/keyboard maps. Deleting an active profile selects another; deleting the final profile is rejected. Save failures restore both maps in RAM.

Hold a startup button while connecting power: GP06 Generic HID, GP08 PS3 USB, GP09 Keyboard, GP10 Xbox BLE, GP11 DirectInput, GP14 setup, GP20 flashing. GP07 is reserved for future Switch and does not activate an implemented transport.

**PS3 USB (experimental)** is a USB-only console test mode with a dedicated 49-byte DS3 report and feature responses adapted from GP2040-CE. Use a data cable to the PS3 and a short GP20 tap for PS. It does not pair over Bluetooth. USB serial is absent in this mode; GP20-at-startup still enters flashing. Motors/motion sensing are not fitted. Console acceptance is pending.

**Xbox BLE:** Windows selects xinputhid.inf but the tested earlier build fails with Code 10 / 0xC00000E5. MiSTer also disconnects just after pairing. The revised build adds Generic Attribute service, encrypted Report Map reads, pairing/subscription/disconnect diagnostics, encryption-state checks and a 30-second incomplete-HID timeout. The user confirms the revised build works in Windows; its BLE driver is healthy and XInput API enumeration succeeds. The exact root cause is not isolated. MiSTer and power-cycle reconnect remain pending. Remove the stale BTPad-Xbox host pairing after flashing, boot with GP10, release, then hold GP20 for three seconds to pair again. Keep USB connected for the serial trace. Verify XInput with tools/test-xinput.ps1 and then test reconnect without sync.

### OLED profile and player line

The left side displays the profile name only, shortened to 12 characters if needed. Beneath BT/USB status, `Player: ?` means unavailable. PS3 USB shows 1–7 when a host player LED command provides that number. Current Bluetooth modes cannot obtain the host player assignment from their reports, so they show `?`; this is not a default Player 1. The old P1/P2/P3 label referred to profiles.

## Xbox Classic Bluetooth test mode

Hold GP12 (R1) at startup and release after boot: OLED XB-BT. Hold GP20 for three seconds to pair BTPad-Xbox-Classic. GP10 still selects Xbox BLE. The portal also offers Xbox Classic BT as a saved default. This experimental Classic Xbox format reuses gamepad profiles, SOCD and turbo; Capture only retains local shortcuts, with no native Capture report. Windows/XP/MiSTer acceptance and reconnect require physical testing. See PROTOCOLS.md for implementation and reference details.
