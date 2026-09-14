# Development tasks

Updated: 2026-09-09. Work in this order; milestone completion requires the gates in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).

2026-09-14 priority update: user confirmed the wiring tester and normal DirectInput work. Correct Generic HID's missing hat and duplicated trigger buttons while preserving DirectInput; add saved-host outgoing reconnect. These changes precede native transport work and need host validation after flashing.

| ID | Priority | Status | Task and completion evidence | Depends on |
| --- | --- | --- | --- | --- |
| BASE-01 | P0 | Done | Review both original Markdown files and sketch; document compatibility gaps, dependencies, and acceptance gates. | — |
| CFG-01 | P0 | Done | Extract shared configuration types without changing fields; validate loaded settings and reject invalid portal edits with rollback. Desktop tests cover all byte values for modes, profile indices, SOCD, turbo rates, and remaps, plus erased data and string bounds. | BASE-01 |
| BUILD-01 | P0 | Done | Exact RP2350B Plus W variant verified in Arduino-Pico 6.1.0; project-local build.ps1 pins ARM/150 MHz/16 MB/IPv4 + Bluetooth and OLED dependencies. Both full firmware builds pass, including GP21 sync and portal changes; sizes and hashes are in VALIDATION.md. | BASE-01 |
| CFG-02 | P0 | Ready | Exercise erased/corrupt EEPROM recovery and failed-write handling on hardware. Do not mark hardware recovery verified from desktop tests. | CFG-01, BUILD-01 |
| WEB-01 | P0 | Device checks pending | Implemented timed session cookies, first-change enforcement, authenticated reads/writes, CSRF tokens, HTML escaping, login throttling and logout. Real handlers pass desktop route tests with transport/storage doubles. Physical browser/network and credential-recovery checks remain. | BUILD-01 |
| INPUT-01 | P1 | Device checks pending | Shared input pipeline implemented and desktop tests pass: remapped last-input SOCD, neutral same-scan ties, release/reactivation, keyboard/gamepad common output, turbo after SOCD, profile history reset, and Special suppression through individual releases. Keyboard releases precede new presses. | BUILD-01 |
| HID-01 | P1 | In progress | User confirmed Windows DirectInput device with all keys and joystick responding on the pairing-fix firmware. Test SOCD/shortcuts on the new build, keyboard mode, reconnect and exact button numbering; labels identify generic transport. | INPUT-01 |
| WEB-02 | P1 | Device checks pending | Implemented separate Load profile / Save settings, optional activation, direct HTML construction and strict settings parsing. Desktop tests cover isolation, missing/malformed fields, escaping, and commit-failure rollback. Browser-on-device check remains. | WEB-01 |
| WEB-TEST-01 | P0 | Device checks pending | User-requested live wiring tester: all 19 physical pins and mapping-table badges highlight while held; authenticated read-only polling, connection recovery and session expiry. Real endpoint and script tests pass. Prioritized for button rewiring before native transport work. | WEB-01 |
| STORE-01 | P1 | Ready | Add versioned storage, integrity checks, migration/recovery, checked commits and deferred writes. | CFG-02, WEB-02 |
| PROFILE-01 | P1 | Ready | Implement independent profile banks per mode and validated import/export; widen turbo mask if Guide/Capture turbo is supported. | STORE-01 |
| BOOT-01 | P1 | Baseline resolved | GP06 Generic HID, GP09 Keyboard, GP14 WLAN; no-button boot uses saved selectable default. Unimplemented console shortcuts disabled. Revisit distinct native-mode shortcuts when XINPUT-01/PS3-01 are implemented. | MODE-01 |
| UX-01 | P2 | Ready | Add pairing reset/reconnect and display scheduling; qualify display-present/absent builds and timing on hardware. | HID-01, PROFILE-01 |
| SYNC-01 | P1 | Device checks pending | User-requested GP21 hold for 3 seconds toggles Bluetooth discovery once per hold; preserves bonds, disconnects current HID host on entry, and suppresses Capture/Touchpad on long holds. Short presses emit on release. Timing tests pass including rollover. Verify actual pairing/reconnect on both transports. | BUILD-01 |
| BT-FIX-01 | P0 | Initial connection verified | Corrected sync-only SSP confirmation/bonding, persistent device names, and mode-specific descriptors to avoid the core's 300-byte SDP buffer overflow. Flashed headless firmware reports pairing success and HID connected. Windows enumerates a 6-axis/32-button joystick. Input accuracy, keyboard mode and power-cycle reconnect remain HID-01/SYNC-01 checks. | SYNC-01 |
| MODE-01 | P0 | Flashed; device checks pending | Removed retired console mode, renamed false XInput choice to Generic HID, disabled unavailable native choices/boot shortcuts, and added tested legacy-mode migration preserving credentials/profiles. LT/RT now also use independent generic trigger axes. Both builds pass. After reconnecting in BOOTSEL, the verified headless image was copied to drive I:; COM10 and live application diagnostics returned. Trigger/migration behavior still needs user testing. | INPUT-01 |
| XINPUT-01 | P0 | Next | Implement a dedicated Xbox-compatible BLE transport/report encoder using the primary references in PROTOCOLS.md. Gate on Windows XInput API enumeration, independent LT/RT and native Guide behavior. Generic HID axes are not completion. | MODE-01 |
| PS3-01 | P1 | Research outlined | Replace placeholder with actual console pairing, feature reports, PS button, input/output and reconnect. See PROTOCOLS.md; requires a console pairing transcript and hardware tests. | MODE-01 |
| PROTO-01 | P2 | Ready | Produce native protocol feasibility matrix with target hardware and acceptance tests; then create one implementation task per feasible mode. | HID-01 |
| RELEASE-01 | P2 | Ready | Record host compatibility, latency, reconnect, power and persistence results; package firmware and setup/recovery instructions. | UX-01, BOOT-01 |

## Known prototype defects and constraints

- User removed PS4 from scope. Native Windows XInput and PS3 do not currently work; unavailable outputs are no longer selectable. Switch remains planned too. Prioritize XINPUT-01 before additional UI polish.
- User smoke-tested password and default-mode changes in the portal and found the initial appearance acceptable. Full mapping/profile editing remains untested on hardware.

- Portal password disclosure and profile overwrite are fixed with desktop handler coverage. Actual HTTP parsing, browser behavior and flash recovery still need device validation; stored passwords remain plaintext.
- SOCD and Special consumption now use the tested shared pipeline. Physical-device checks remain; shortcut changes still persist immediately to flash (deferred/wear-conscious writes remain STORE-01).
- Turbo storage currently supports only A through RS. The first fix disables unsupported checkboxes and ignores unsupported turbo fields.
- Desktop tests cover configuration validation, portal handlers, GP21 timing and the shared input pipeline. User confirmed Windows pairing and basic gamepad inputs. Hardware flash recovery, keyboard/reconnect, SOCD/shortcut behavior, browser/network integration and latency need further verification.
- The workspace is now a local Git project named BT-GP2350B-Plus-W-CE Bluetooth Controller Firmware. Release packaging and tags remain RELEASE-01 work.
