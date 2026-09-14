# Validation record

Date: 2026-09-09. Firmware is a development prototype. The headless build was flashed to the connected RP2350 with user authorization.

## Latest source and build status

The shared SOCD/Special input update was built and flashed successfully, and USB status confirmed it running. Its headless SHA-256 was `9971B00C94BB0507E7561924E8A1A6AF3959C49E7F1BD40CE040869AE73EAAF3` (509,196 program bytes, 96,084 static RAM bytes). Physical SOCD/shortcut acceptance remains pending.

Following user feedback, the latest source additionally removes the retired console mode, renames generic output honestly, rejects unavailable native choices, migrates saved legacy modes without wiping credentials/profiles, and sends LT/RT on independent generic Z/Rz axes. Both builds pass:

| Latest build | Program bytes | Static RAM bytes | SHA-256 |
| --- | --- | --- | --- |
| Headless | 509,324 | 96,084 | `69B313E55B1B6A2B03615DBC580463E898DFC301844580C797CF66B162D5C92C` |
| OLED | 518,024 | 96,368 | `563EBC619CC2D2BC4F2B7728A0809AAAFBD784C155D2A351450C6DF11BB50836` |

The first upload attempt found no board. After the user reconnected in BOOTSEL, drive I: identified as RP2350 and the latest headless checksum above matched. Copy completed successfully; I: disappeared, COM10 returned, and live diagnostics reported BT sync=0 connected=0 pairing=0xff auth=0xff. The latest firmware is flashed and running; trigger axes, saved-mode migration, SOCD and shortcut behavior remain to be tested on hardware.

All four desktop suites pass, including the new shared input tests, suppressed GP21 gestures, rejected native-mode saves and credential-preserving migration checks. Latest build logs: `.build/compile-modes-headless.log` and `.build/compile-modes-display.log`. Historical sizes/hashes below describe earlier revisions.

The user found the web interface acceptable at first glance and tried changing password/default mode only. This is a limited smoke test; mapping/profile editing and broader browser/storage behavior still need hardware checks.

## Flash result

Verified `I:/INFO_UF2.TXT` reported the RP2350 bootloader and checked the headless UF2 against the SHA-256 below before copying. The copy completed successfully, drive I: disappeared, and Arduino CLI subsequently listed USB serial COM10 with Waveshare RP2350B Plus W among its matching board definitions. USB identifiers match multiple boards, so this enumeration alone does not identify the exact board or prove Bluetooth operation. Pairing, inputs and portal operation remain to be tested.

## Windows pairing correction

The user reported that the initial device was discoverable but Windows pairing timed out. Source inspection of Arduino-Pico 6.1.0 found:

- BTstack defaults `ssp_auto_accept` to zero; the HID helper observes confirmation requests but does not reply. Sync now enables Just Works auto-confirmation and bonding, and disables both outside sync.
- Linking both helpers combines a 70-byte gamepad descriptor with a 92-byte keyboard/consumer descriptor. With the initial 12-character name, the generated SDP record is 361 bytes against a 300-byte helper buffer. Mode-specific descriptors now fit, with compile-time size guards. Descriptor sizes were verified from the built ELF.
- `gap_set_local_name` retains its argument, but startup previously passed a temporary String's buffer. Names now have static lifetime.

The corrected headless build uses 508,988 program bytes and 96,068 static RAM bytes. SHA-256: `931FE92C00B4986F61AABA14EBF8B762E250F0BBBF8C92CCD522B04EF815873E`. It was uploaded through COM10; the uploader reset the board, wrote 1,067,008 UF2 bytes via I:, and detected COM10 again. USB diagnostics then reported `BT sync=0 connected=0 pairing=0xff auth=0xff`, confirming the corrected application runs. The subsequent pairing result is recorded below. Logs: `.build/compile-pairing-fix.log`, `.build/pairing-status.log`.

The OLED variant was also rebuilt successfully with the correction: 517,680 program bytes, 96,356 static RAM bytes; SHA-256 `B9F0F90CEF50B060918C052BFAE41703BEFEAA63A8CB386B90045F5F51FC5CFD`. It was not flashed. The build table and hashes below retain the original baseline for comparison; both corrected artifacts supersede that baseline.

During the retry, USB diagnostics transitioned from sync off to sync on and then `BT sync=1 connected=1 pairing=0x00 auth=0xff`. This confirms successful Simple Pairing and an open HID connection; `auth=0xff` means the separate authentication event was not observed, not an authentication failure. A Windows joystick API query subsequently enumerated one controller with six axes and 32 buttons, consistent with the gamepad descriptor. Full input mapping and reconnect are not yet verified.

## Build configuration

- Arduino CLI 1.2.0; Arduino-Pico 6.1.0, installed under `.build/arduino-data`.
- Board: `rp2040:rp2040:waveshare_rp2350b_plus_w:arch=arm,freq=150,flash=16777216_0,ipbtstack=ipv4btcble`.
- Exact core variant selects RP2350B, with RM2 radio on GPIO36–39. Controller GPIO02–21 and display GPIO00–01 do not overlap the radio pins.
- Optional OLED libraries: Adafruit BusIO 1.17.4, GFX 1.12.6, SSD1306 2.5.17.
- Reproduction: `./build.ps1 -Install` and `./build.ps1 -Install -WithDisplay`; omit `-Install` after setup. Project-local dependencies leave the global Arduino package setup unchanged.
- Logs: `.build/compile-headless.log`, `.build/compile-display.log`.

Both final builds pass, including the portal and GP21 sync changes:

| Build | Program bytes | Static RAM bytes | UF2 |
| --- | --- | --- | --- |
| Without OLED | 509,340 | 96,068 | `.build/headless/btpad_pico.ino.uf2` |
| With OLED | 517,960 | 96,356 | `.build/display/btpad_pico.ino.uf2` |

Available program space is 16,769,024 bytes; RAM is 524,288 bytes. Static figures do not include peak runtime allocations or stack usage. Builds use all compiler warnings; upstream Arduino-Pico/library warnings remain (including hidden virtual methods and unused parameters). No project-source warning was observed.

SHA-256 of generated UF2 files:

```text
headless B8FA26AAADD5D384A559F8346F91550D67792E0A9585B67F8F83880F33897C00
display  B5DBE102FAB82ED47B684EF0C68964ACF1620B91929531611B8327193D1730C5
```

## Automated tests

`./tests/run.ps1` compiles with Visual Studio C++ `/W4 /WX` and runs:

- Configuration: erased data, invalid modes/profile indices, unterminated strings, all byte-valued remaps/turbo rates/SOCD values, reserved Special mapping, and turbo-mask bounds.
- Portal: unauthorized access, login CSRF/rate limiting, required initial password replacement, cookie flags, password-change session invalidation, strict settings parsing, selected-profile isolation/activation, HTML escaping, commit-failure rollback, forged cookies, logout, expiry, and millisecond counter rollover.
- GP21: exact 3-second threshold, one toggle per hold, rearming after release, no Capture tap after a long hold, short tap emitted once, and timer rollover.

All three desktop suites pass. Portal tests execute the actual firmware route handlers with test doubles for HTTP, storage, time, and entropy; they do not validate the real server parser, radio, flash, or browser rendering. The test entropy stub is only on the desktop test include path; firmware uses Pico SDK entropy.

## Required device checks

User confirmation after the pairing correction: Windows sees a DirectInput pad and all wired keys and joystick respond. This establishes basic input operation, not yet keyboard mode, SOCD edge cases, or reconnect durability.

1. Record board revision, host OS/console version, firmware hash, wiring and power source.
2. Boot the headless image, hold GP21 for three seconds, and verify discovery appears. Hold again after release and verify it disappears. Repeat in keyboard mode. Confirm known-host reconnect, disconnect on sync entry, and retained bonds. Check that long holds cause no Capture/Touchpad action and short taps produce one action.
3. Hold GP14 at boot, connect to BTPad-Setup using the current saved password (initially changeme), and visit http://192.168.4.1/. Verify only sign-in is visible before authentication, initial password replacement is mandatory, and password change signs out. Restart and reconnect Wi-Fi using the new password.
4. Load profile 2, edit/save it without activating, reload all profiles and power-cycle. Verify other profiles remain unchanged. Repeat with activation, mappings, SOCD and turbo. Check unauthorized and expired-session saves on the actual server.
5. Test both OLED-present and OLED-absent configurations; confirm GPIO00/01 display wiring, visible sync status and acceptable input timing.
6. Verify erased/corrupted EEPROM recovery, failed commit behavior and interrupted power during save. Desktop rollback does not establish power-loss safety. Versioned storage and a physical password-recovery mechanism remain STORE-01 work.

Native Xbox, Switch and PlayStation wireless compatibility remains unimplemented. Generic HID host compatibility, latency and stability must be measured on hardware.

## Live wiring tester — 2026-09-09

User subsequently confirmed that the wiring tester works on hardware.

All four desktop C++ suites pass. The actual `/inputs` route is tested for unauthenticated/initial-password/expired-session rejection, every individual physical pin, simultaneous opposite directions, release, and absence of configuration writes. The external script test (`node tests/wiring_test.cjs`) passes highlights in duplicate badges, connection failure/recovery, and session expiry. These use simulated transport/DOM, not a physical browser test.

Headless build passes: 509852 bytes program, 96084 bytes static RAM. UF2 SHA256: `3C9C8036A699EDC7A4348925131F1279F3D0C387E447CCE5BD3ADE8DB4B92220`. The built ELF contains the tester page and script. After the user reconnected the board, verified the RP2350 bootloader identity on I: and copied this hash-verified image successfully. COM10 returned, with two live application diagnostic lines: `BT sync=0 connected=0 pairing=0xff auth=0xff`. Physical web/GPIO testing remains pending.

On device: enter setup with GP14 at boot, sign in, hold each input including GP14/GP21, and check both tester and mapping-table labels turn green and show Pressed. Verify release and simultaneous directions; unplug or disconnect Wi-Fi and check highlights clear with a reconnect message. Confirm no settings change during testing.

OLED build also passes: 522648 bytes program, 96368 bytes static RAM. Logs: `.build/compile-wiring-headless.log` and `.build/compile-wiring-display.log`.

## Generic HID and reconnect correction — 2026-09-14

User reports normal DirectInput working, Generic HID missing D-pad on some hosts and duplicating triggers as buttons, and reconnect requiring sync. Generic HID now sends a POV hat with centered X/Y, and Z/Rz triggers without button 7/8. DirectInput's prior output is preserved. Released trigger axes remain at minimum by design; no connected switch is needed for that resting value.

Added connectable mode outside discovery and outgoing attempts to saved bonds. Attempts rotate saved hosts after at least 10 idle seconds and are blocked while syncing, connected, stack not ready, or any ACL is pending/active. Bonds are persisted by the core TLV implementation, but its flash banks are inside the firmware image and can be overwritten by reflashing.

All six C++ suites pass, including new report and reconnect tests against the actual headers with stack doubles. Coverage includes all eight hat directions and neutral, independent/simultaneous triggers, unchanged DirectInput, bond rotation, no bonds, pending connection protection, sync/connected gating and clock rollover. Headless build passes: 510460 bytes program, 96092 bytes static RAM; SHA256 `9F903859128C22C7066C05D21255A9BAD69A02F526BE149C5087094889C5B86D`. Log: `.build/compile-reconnect-headless.log`. Board absent during build; flashing and hardware validation pending.

Device checks: pair once after flash, select Generic HID, verify the Windows POV indicator in all directions and no button 7/8 from triggers, then repeat MiSTer Define joystick buttons. Power-cycle without sync and allow saved-host reconnect; repeat with host initially off then on, and after a dropped connection. Confirm normal DirectInput still works. No successful physical reconnect is claimed from mocked scheduling tests.

OLED build also passes: 523288 bytes program, 96376 bytes static RAM; SHA256 `2FA33996837C0B5C91850F2128B15553B59F8D01D320C561C2A4683470CBE980`. Log: `.build/compile-reconnect-display.log`.

2026-09-14 flash completed after the user reconnected the board. Verified RP2350 bootloader identity on I: and headless SHA256 `9F903859128C22C7066C05D21255A9BAD69A02F526BE149C5087094889C5B86D`, then copied the image successfully. COM10 returned with two live `BT sync=0 connected=0 pairing=0xff auth=0xff` diagnostic lines. Firmware startup is confirmed; host input and automatic reconnect still require physical testing.

## OLED and GP20 update — 2026-09-14

Runtime sync moved from GP21 to GP20 (three seconds); Guide emits only on short release. GP21 now remains active while held as normal Capture. GP20 sampled low at startup enters the RP2350 ROM USB loader before storage, display or Bluetooth initialization. This entry path is only called from setup; a runtime hold cannot flash/reboot. Existing EEPROM IDs and mappings are unchanged.

The SSD1306 display now uses an original fixed layout inspired by [GP2040-CE display functions](https://gp2040-ce.info/web-configurator/menu-pages/display-configuration/): mode, profile, Bluetooth status, physical D-pad and eight main inputs, seven auxiliary indicators, turbo rings/rate and SOCD. Setup mode gives AP and URL. Frames are limited by elapsed time to 10 Hz; I2C address 0x3C is probed before initialization. Full frame transfer remains synchronous, so actual input latency with OLED still needs measurement. The GP2040-CE mini-menu, configurable layouts, splash and history are not implemented.

All eight C++ suites plus the wiring-script suite pass. New tests cover GP20 startup samples, short/long/suppressed runtime gestures, Guide pulse routing, ordinary GP21 holds, and text/shape bounds for all supported OLED modes, longest profile, turbo rings and all 19 physical inputs.

OLED build: 524696 bytes program, 96380 bytes static RAM. UF2 SHA256 `5D2A69F2E557A0DC45098166AABE318297E22D012BC6D2D5581B7F132F2476CC`. Log `.build/compile-gp20-display.log`. Board was absent; flashing and physical OLED/boot/sync tests pending. Use the physical BOOTSEL button for this first update, then verify GP20 held on power-up presents the RP2350 drive; also verify normal boot, runtime three-second sync, short Guide, GP21 held input and GP14 web mode.

Headless build also passes: 510500 bytes program, 96092 bytes static RAM. SHA256 `39301EB8E1661C4713D82E2FF83F48D7AA99B87C3E26C32015E19DC40D7BEC57`. Log `.build/compile-gp20-headless.log`. Use the OLED image for the user's now-fitted display.

After the user connected the board, verified RP2350 bootloader identity on I: and OLED SHA256 `5D2A69F2E557A0DC45098166AABE318297E22D012BC6D2D5581B7F132F2476CC`, then copied the OLED UF2 successfully. COM10 returned with two live `BT sync=0 connected=0 pairing=0xff auth=0xff` diagnostic lines. Startup is confirmed; OLED appearance, GP20 physical boot/sync actions and GP21 behavior still require user checks.
