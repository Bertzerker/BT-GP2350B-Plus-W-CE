# Development tasks

Updated: 2026-09-09. Work in this order; milestone completion requires the gates in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).

2026-09-14 priority update: user confirmed the wiring tester and normal DirectInput work. Correct Generic HID's missing hat and duplicated trigger buttons while preserving DirectInput; add saved-host outgoing reconnect. These changes precede native transport work and need host validation after flashing.

| ID | Priority | Status | Task and completion evidence | Depends on |
| --- | --- | --- | --- | --- |
| BASE-01 | P0 | Done | Review both original Markdown files and sketch; document compatibility gaps, dependencies, and acceptance gates. | — |
| WEB-MOBILE | P1 | Browser checked; device checks pending | Removed duplicate tester box; all 19 GPIO indicators appear once in Profile mapper, with fixed Special row. Added phone-friendly controls and user favicon/background resized for embedded serving. Mock browser verified at 320/390/1024px; endpoint/script tests pass. | WEB-TEST-01 |
| OLED-GP20 | P0 | Hardware checks pending | User fitted OLED. Added GP2040-CE-inspired status/live-input layout and elapsed-time refresh, moved runtime sync to GP20, and added GP20-at-startup ROM USB flashing. GP21 restored as ordinary Capture. Desktop routing, timing, boot and display bounds tests pass. | BUILD-01 |
| CFG-01 | P0 | Done | Extract shared configuration types without changing fields; validate loaded settings and reject invalid portal edits with rollback. Desktop tests cover all byte values for modes, profile indices, SOCD, turbo rates, and remaps, plus erased data and string bounds. | BASE-01 |
| BUILD-01 | P0 | Done | Exact RP2350B Plus W variant verified in Arduino-Pico 6.1.0; project-local build.ps1 pins ARM/150 MHz/16 MB/IPv4 + Bluetooth and OLED dependencies. Both full firmware builds pass, including GP20 sync and portal changes; sizes and hashes are in VALIDATION.md. | BASE-01 |
| CFG-02 | P0 | Ready | Exercise erased/corrupt EEPROM recovery and failed-write handling on hardware. Do not mark hardware recovery verified from desktop tests. | CFG-01, BUILD-01 |
| WEB-01 | P0 | Device checks pending | Implemented timed session cookies, first-change enforcement, authenticated reads/writes, CSRF tokens, HTML escaping, login throttling and logout. Real handlers pass desktop route tests with transport/storage doubles. Physical browser/network and credential-recovery checks remain. | BUILD-01 |
| INPUT-01 | P1 | Device checks pending | Shared input pipeline implemented and desktop tests pass: remapped last-input SOCD, neutral same-scan ties, release/reactivation, keyboard/gamepad common output, turbo after SOCD, profile history reset, and Special suppression through individual releases. Keyboard releases precede new presses. | BUILD-01 |
| HID-01 | P1 | In progress | User confirmed Windows DirectInput device with all keys and joystick responding on the pairing-fix firmware. Test SOCD/shortcuts on the new build, keyboard mode, reconnect and exact button numbering; labels identify generic transport. | INPUT-01 |
| WEB-02 | P1 | Device checks pending | Implemented separate Load profile / Save settings, optional activation, direct HTML construction and strict settings parsing. Desktop tests cover isolation, missing/malformed fields, escaping, and commit-failure rollback. Browser-on-device check remains. | WEB-01 |
| WEB-TEST-01 | P0 | Device checks pending | User-requested live wiring tester: all 19 physical pins and mapping-table badges highlight while held; authenticated read-only polling, connection recovery and session expiry. Real endpoint and script tests pass. Prioritized for button rewiring before native transport work. | WEB-01 |
| STORE-01 | P1 | Ready | Add versioned storage, integrity checks, migration/recovery, checked commits and deferred writes. | CFG-02, WEB-02 |
| PROFILE-01 | P1 | Ready | Retain shared logical gamepad profiles and separate keyboard maps; implement validated import/export; widen turbo mask if Guide/Capture turbo is supported. | STORE-01 |
| BOOT-01 | P1 | Baseline resolved | GP06 Generic HID, GP09 Keyboard, GP14 WLAN; no-button boot uses saved selectable default. Unimplemented console shortcuts disabled. Revisit distinct native-mode shortcuts when XINPUT-01/PS3-01 are implemented. | MODE-01 |
| UX-01 | P2 | Ready | Add pairing reset/reconnect and display scheduling; qualify display-present/absent builds and timing on hardware. | HID-01, PROFILE-01 |
| SYNC-01 | P1 | Device checks pending | User-requested GP20 hold for 3 seconds toggles Bluetooth discovery once per hold; preserves bonds, disconnects current HID host on entry, and suppresses Guide on long holds. Short presses emit on release. Timing tests pass including rollover. Verify actual pairing/reconnect on both transports. | BUILD-01 |
| BT-FIX-01 | P0 | Initial connection verified | Corrected sync-only SSP confirmation/bonding, persistent device names, and mode-specific descriptors to avoid the core's 300-byte SDP buffer overflow. Flashed headless firmware reports pairing success and HID connected. Windows enumerates a 6-axis/32-button joystick. Input accuracy, keyboard mode and power-cycle reconnect remain HID-01/SYNC-01 checks. | SYNC-01 |
| MODE-01 | P0 | Flashed; device checks pending | Removed retired console mode, renamed false XInput choice to Generic HID, disabled unavailable native choices/boot shortcuts, and added tested legacy-mode migration preserving credentials/profiles. LT/RT now also use independent generic trigger axes. Both builds pass. After reconnecting in BOOTSEL, the verified headless image was copied to drive I:; COM10 and live application diagnostics returned. Trigger/migration behavior still needs user testing. | INPUT-01 |
| XINPUT-01 | P0 | Hardware acceptance pending | Dedicated Xbox BLE ATT service and tested 16-byte encoder implemented. Gate on Windows XInput API enumeration, independent LT/RT and native Guide behavior. Generic HID axes are not completion. | MODE-01 |
| PS3-01 | P1 | Research outlined | Replace placeholder with actual console pairing, feature reports, PS button, input/output and reconnect. See PROTOCOLS.md; requires a console pairing transcript and hardware tests. | MODE-01 |
| PROTO-01 | P2 | Ready | Produce native protocol feasibility matrix with target hardware and acceptance tests; then create one implementation task per feasible mode. | HID-01 |
| RELEASE-01 | P2 | Ready | Record host compatibility, latency, reconnect, power and persistence results; package firmware and setup/recovery instructions. | UX-01, BOOT-01 |

## Known prototype defects and constraints

- User removed PS4 from scope. Xbox BLE is experimental and selectable for testing; Windows XInput recognition is unverified. PS3 remains unavailable. Switch remains planned too. Prioritize XINPUT-01 before additional UI polish.
- User smoke-tested password and default-mode changes in the portal and found the initial appearance acceptable. Full mapping/profile editing remains untested on hardware.

- Portal password disclosure and profile overwrite are fixed with desktop handler coverage. Actual HTTP parsing, browser behavior and flash recovery still need device validation; stored passwords remain plaintext.
- SOCD and Special consumption now use the tested shared pipeline. Physical-device checks remain; shortcut changes still persist immediately to flash (deferred/wear-conscious writes remain STORE-01).
- Turbo storage currently supports only A through RS. The first fix disables unsupported checkboxes and ignores unsupported turbo fields.
- Desktop tests cover configuration validation, portal handlers, GP20 timing and the shared input pipeline. User confirmed Windows pairing and basic gamepad inputs. Hardware flash recovery, keyboard/reconnect, SOCD/shortcut behavior, browser/network integration and latency need further verification.
- The workspace is now a local Git project named BT-GP2350B-Plus-W-CE Bluetooth Controller Firmware. Release packaging and tags remain RELEASE-01 work.

## 2026-09-15 implementation update

- Shared gamepad mapping names now follow btpad.md, independent of output mode; DirectInput face numbering follows that table.
- Separate full keyboard map per profile, checksummed storage extension, validated save and rollback, modifiers and six-key rollover implemented.
- Sync automatically closes after HID connection. Classic reconnect clears stalled established ACL links after a timeout and reports bond counts. BLE keeps advertising for bonded-host reconnect. Hardware reconnect remains open.
- Xbox BLE descriptor/report tests pass; native Windows XInput, Guide, reconnect and pairing must be verified before calling this supported.

## 2026-09-20 follow-up

- Implemented profile create/delete within three slots, keyboard-map reset/isolation, active-profile fallback, last-profile protection and rollback tests.
- Implemented requested Capture/Special shortcuts and release suppression; desktop tests pass.
- Added startup selections for every selectable output: GP06 Generic, GP08 PS3 USB, GP09 Keyboard, GP10 Xbox BLE, GP11 DirectInput. GP07 Switch reserved until implemented.
- Added experimental PS3 USB descriptor/report/features with desktop report tests. Console test remains pending; PS3 Bluetooth remains open.
- Xbox BLE failed user tests on Windows (Code 10) and MiSTer (immediate disconnect). Added diagnostics/recovery and revised GATT database. Root cause and fix require live trace, not assumptions from Windows driver matching.

2026-09-20 hardware follow-up: user confirms the flashed Xbox BLE build works and stays connected. Windows BLE XInput driver status OK and XInputGetState slot 0 observed. Keep MiSTer, Guide/trigger acceptance and repeated automatic reconnect open. PS3 USB console test remains pending.

## 2026-09-21 player assignment

- Implemented Special GP14 + Home/Guide GP20 reconnect, with host-owned player slots and existing Bluetooth bonds/identity retained. PS3 uses USB reconnect. Does not guarantee Player 1 or cycle slots.
- Input/sync tests and OLED/headless builds pass; OLED image flashed. Physical shortcut checks on Windows and MiSTer remain open. Post-flash serial showed BLE pairing failures (0x3d); reconnect acceptance remains unverified. See VALIDATION.md.

## 2026-09-21 OLED player feedback

- Profile name only on the left; host player indicator below the connection status on the right.
- PS3 USB decodes host player LED reports (1–7). Other current protocols show unknown (`Player: ?`) because they do not provide player-number feedback.
- Actual Xbox BLE player display on Windows/MiSTer still needs host integration; no slot is inferred from profiles, pairing order or connection handles.
- OLED bounds and PS3 output decoding tests pass, including truncated reports and unknown LED patterns. Physical PS3 LED-feedback acceptance remains pending.

## 2026-09-22 Xbox Classic Bluetooth

- Implemented separate experimental mode 8 with GP12/R1 startup shortcut, portal default selection and XB-BT display label.
- Dedicated BTstack Classic HID transport, matching report encoder, large SDP record and 045E:02E0 Device ID; pinned Apache-2.0 descriptor reference retained.
- Shared profile/SOCD/turbo input and reconnect shortcut preserved; Capture has local shortcuts only in this older report family.
- 12 native suites and OLED/headless builds pass. OLED firmware flashed.
- Open: select GP12 mode and verify Classic Windows pairing/XInput, Guide, triggers, automatic reconnect, MiSTer and XP. Initial post-flash boot still uses saved BLE mode.

Classic hardware follow-up: user confirmed sync; Windows BTPad-Xbox-Classic and matching Classic XInput driver are healthy. XInput slot 0 shows buttons and isolated LT/RT presses without duplicate button bits. Remaining: full D-pad/Guide, simultaneous triggers, power-cycle reconnect, MiSTer and XP.
