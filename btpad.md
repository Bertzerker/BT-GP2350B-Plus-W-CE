# BTPad product requirements

Build wireless gamepad firmware inspired by GP2040-CE for Waveshare RP2350B-Plus-W. Use [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for sequencing and [TASKS.md](TASKS.md) for status.

## Supported targets and scope

- Implement native Windows XInput over Bluetooth, Switch, PS3, generic DirectInput/HID, and Keyboard as distinct protocols.
- PS4 was removed from project scope at the user's request on 2026-09-09.
- Current firmware implements generic HID and keyboard only. Native labels must not be exposed as working modes until tested on their intended hosts.
- XInput acceptance requires real Windows XInput enumeration, independent trigger values and native Guide behavior (Game Bar/Steam subject to host settings). Ordinary button 13 is insufficient.
- PS3 currently does not work on the console; implement its actual pairing, feature reports and input protocol.

## GPIO mapping

Buttons are active-low to ground. The labels below specify target behavior across requested protocols.

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
## Controls and configuration

1. GP06 selects the working Generic HID mode at boot; GP09 selects Keyboard, GP14 selects WLAN setup. Native-mode boot selections return only when implemented. The original GP06 XInput/DirectInput conflict is superseded by this explicit baseline.
2. Hold GP20 for 3 seconds to toggle Bluetooth sync. Short taps retain Guide; a sync hold must not emit that host input.
3. Capture (GP21) + Left/Right selects the previous/next profile; Capture + Up/Down cycles SOCD forward/backward. Special (GP14) + Up/Down raises/lowers turbo speed; Special + a game button toggles its turbo. Suppress shortcut inputs through release.
4. WLAN setup hosts an access point and requires changing the initial password. Gamepad profiles use the Maps to column as a shared logical layer across output protocols, with common turbo and SOCD settings. Each profile also has an independent physical-input keyboard map supporting all normal keyboard keys.
5. Support neutral, up-priority and last-input SOCD after remapping, consistently in keyboard and gamepad outputs.
6. Optional I2C display on GP00/GP01 shows input state, turbo, profile, output mode and Bluetooth sync status.

User smoke tests: Windows generic pad pairs and all wired controls respond. The setup page looked usable; only password and default-mode changes have been tried so far. Full profile editing and persistence tests remain.
