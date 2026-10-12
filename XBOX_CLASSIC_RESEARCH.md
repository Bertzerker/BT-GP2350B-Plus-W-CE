# Legacy 8BitDo / Xbox-compatible Bluetooth research

Research date: 2026-09-21. Research only; no controller firmware changed or flashed for this investigation.

## Finding

The strongest next firmware experiment is a separate **Xbox Classic Bluetooth (BR/EDR HID)** mode, alongside the working Xbox BLE mode. This is a credible route to reproducing legacy 8BitDo's direct Bluetooth XInput behavior. It is not yet a demonstrated Windows XP solution, and it is not the original Xbox 360 proprietary wireless protocol.

The user reports that legacy Bluetooth 8BitDo controllers work in Windows XP after installing Xbox 360 Accessories drivers. Retain this as hardware-test evidence; the exact XP Bluetooth stack, connection path, controller firmware and bound driver have not been captured. Do not dismiss that result or silently substitute a USB receiver explanation.

## Evidence

- [8BitDo SN30/SF30 v4 manual](https://download.8bitdo.com/Manual/Controller/SN30%2BSF30/SN30%2BSF30_Manual_V4.pdf?20220513=) documents Windows XInput mode and pairing through Windows Bluetooth settings. Direct Bluetooth XInput is therefore an explicit manufacturer feature, not just a receiver feature.
- [8BitDo SN30 Pro+ FAQ](https://support.8bitdo.com/faq/sn30-pro-plus.html) calls the mode “X-input mode (Xbox 360 mode).” Its published compatibility list specifies Windows 10; that list does not establish the mechanism of the user's XP success.
- A [first-hand SNES30 device enumeration in RetroArch's issue tracker](https://github.com/libretro/RetroArch/issues/8857) reports Classic HID service 0x1124, USB-vendor-ID source, VID 0x045E/PID 0x02E0 and the Windows Bluetooth XINPUT driver. The issue's description calls this Xbox 360, but the identifier is the Xbox One Bluetooth family: [Bluepad32's parser](https://github.com/ricardoquesada/bluepad32/blob/main/src/components/bluepad32/parser/uni_hid_parser_xboxone.c) explicitly identifies 0x02E0 as Xbox One Bluetooth and contains a Classic-era descriptor.
- Read-only inspection of this PC independently found **8BitDo SN30 Pro+**, its Device Identification Service, and its Bluetooth XINPUT compatible input device sharing one Bluetooth address. Its identity is also 0x045E:0x02E0 on BTHENUM. This proves the local model/transport association, not its behavior on XP.
- This PC's `C:/Windows/INF/xinputhid.inf` binds 0x045E:0x02E0 through BTHENUM (Classic), while our present 0x045E:0x0B13 uses BTHLEDevice (BLE). The local `xusb22.inf` instead lists USB Xbox 360 IDs and USB XUSB compatible IDs. These are current-PC observations, not an inspection of XP's accessory driver package.
- [Microsoft's HID transport documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-transports) dates inbox HID-over-BLE support to Windows 8. Our current BLE implementation therefore does not supply an inbox XP path.

## What to implement experimentally

1. Capture the SN30 Pro+ in XInput mode: Classic SDP HID and Device ID records, HID descriptor, report IDs/lengths, neutral/button/trigger/hat/Guide input, initialization reads/writes and reconnect behavior. Capture the tested older controller/firmware as well if its descriptor differs. Start with observation; do not clone a controller's Bluetooth address.
2. Add a dedicated BTstack Classic HID transport with correctly sized SDP storage and the observed Device ID source/VID/PID. Implement the matching input and host output/feature behavior, including Guide and any battery/rumble reports actually present. Merely changing a name or VID/PID cannot provide compatibility.
3. Feed it the existing neutral logical input pipeline so profiles, SOCD, turbo and keyboard storage remain independent. Use raw mode-specific reports; do not route Xbox Classic packets through JoystickBT's generic encoder. Keep the tested Xbox BLE mode selectable, and preserve mode/storage compatibility.
4. Implement Classic pairing restricted to GP20 sync, saved-host reconnection outside sync, and Special+Home reconnect. Verify both host-initiated and controller-initiated reconnect. Do not infer player numbers from profile indices or transport handles; inspect output reports before promising player feedback.

Local implementation constraints: Arduino-Pico 6.1.0's `PicoBluetoothHID` hard-codes BlueKitchen Device ID information and provides only a 300-byte HID SDP buffer. A full Xbox-family descriptor plus SDP attributes needs a deliberately sized record. Prefer a project-owned transport over modifying the cached core. Bluepad32 is a host/parser reference, not a drop-in peripheral implementation; review license and pin the selected source revision before adapting code.

## XP acceptance and the unresolved detail

The next decisive evidence is the working controller's **device instance path, bound driver/INF, driver files, Bluetooth adapter/stack and controller firmware version on XP**. Record whether it is paired through the OS Bluetooth stack or an adapter that presents a USB controller. The controller model alone does not establish this path.

Check native XInput input, rather than the display name alone. Test independent and simultaneous triggers, hat diagonals, Guide, restart/reconnect and two-controller order. XP needs an XP-compatible XInput diagnostic; the current `tools/test-xinput.ps1` uses xinput1_4.dll and is not an XP test tool. Test MiSTer separately with the Retro-Bit receiver unplugged, then reconnect it to measure ordering interactions.

If the XP setup uses a software or receiver translation layer, reproducing its over-air HID descriptor alone will not reproduce that whole system. That question remains open pending the working XP device/driver evidence.

## Alternative if a USB receiver is acceptable

A Bluetooth-to-USB bridge can receive BTPad wirelessly and expose an Xbox 360-compatible USB device to the PC. The USB side would use an actual XUSB implementation, not a generic HID descriptor. [tinyusb-xinput](https://github.com/fluffymadness/tinyusb-xinput) provides an RP2040 reference; its descriptor uses vendor-specific interface class 0xFF, subclass 0x5D, protocol 0x01, with interrupt IN/OUT endpoints. This is a reference architecture, not verified XP/MiSTer support for our project.

For that design, first validate wired USB XUSB enumeration on XP with the accessory driver, then add a Bluetooth host receiver and bidirectional forwarding. Player LED output could be relayed to the pad if present. The receiver should stop exposing an active controller when the pad disconnects, specifically to avoid the persistent occupied-slot behavior observed with Retro-Bit. Receiver disconnect/re-enumeration policy and latency require hardware tests.

The preferred next step for a **direct Bluetooth** goal is to capture and reproduce the observed legacy 8BitDo Classic HID behavior. Treat the receiver bridge as a separate design choice, not a prerequisite already established by the evidence.
