/* Bluetooth/USB controller firmware for Waveshare RP2350B-Plus-W.
 * Classic Generic HID/DirectInput/keyboard, experimental Xbox BLE and PS3 USB.
 * See README.md and PROTOCOLS.md for host validation and known limitations.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <JoystickBT.h>
#include <HID_Bluetooth.h>
#include <pico/bootrom.h>
#include <hardware/watchdog.h>
#include <hardware/structs/watchdog.h>
#include "UsbSetupCommand.h"

#if defined(BTPAD_REQUIRE_DISPLAY) || (!defined(BTPAD_DISABLE_DISPLAY) && __has_include(<Wire.h>) && __has_include(<Adafruit_GFX.h>) && __has_include(<Adafruit_SSD1306.h>))
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define BTPAD_HAS_DISPLAY 1
#else
#define BTPAD_HAS_DISPLAY 0
#endif

#if defined(BTPAD_REQUIRE_DISPLAY) && !BTPAD_HAS_DISPLAY
#error "Display build requires Adafruit GFX and SSD1306 libraries. Run build.ps1 -Install -WithDisplay."
#endif

#include "Config.h"

#include "BoardInputs.h"
#include "SyncButton.h"
#include "InputPipeline.h"
#include "BluetoothReports.h"
#include "GamepadOutput.h"
#include "DisplayUI.h"
#include "BootControl.h"
#include "BootModes.h"
#include "PS3USB.h"
#include "KeyboardMap.h"

constexpr size_t EEPROM_SIZE = sizeof(Config) + sizeof(KeyboardConfig);
constexpr uint16_t DEBOUNCE_MS = 5;
constexpr uint16_t REPORT_MS = 4;

Config config;
KeyboardConfig keyboardConfig;
HidMode currentMode = MODE_GAMEPAD;
WebServer server(80);

bool rawState[IN_COUNT] = {};
bool stableState[IN_COUNT] = {};
InputPipeline inputPipeline;
uint32_t logicalState = 0;
uint32_t lastChangeMs[IN_COUNT] = {};


uint32_t lastReportMs = 0;
bool turboPhase = false;
uint32_t lastTurboFlipMs = 0;
SyncButton syncButton;
bool bluetoothSync = false;
bool syncTap = false;
static btstack_packet_callback_registration_t pairingEvents;
static uint8_t pairingStatus = 0xff, authenticationStatus = 0xff;
#include "XboxBLE.h"
#include "XboxClassic.h"

bool bluetoothConnected() {
  if (currentMode == MODE_PS3) return PS3USB::connected();
  if (currentMode == MODE_XBOX_CLASSIC) return XboxClassic::connected();
  return currentMode == MODE_XBOX_BLE ? XboxBLE::connected() : PicoBluetoothHID.connected();
}

void observePairing(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
  (void)channel;
  if (type != HCI_EVENT_PACKET || size < 3) return;
  switch (hci_event_packet_get_type(packet)) {
    case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
      pairingStatus = hci_event_simple_pairing_complete_get_status(packet);
      break;
    case HCI_EVENT_AUTHENTICATION_COMPLETE:
      authenticationStatus = hci_event_authentication_complete_get_status(packet);
      break;
  }
}

void setBluetoothSync(bool enabled) {
  if (currentMode == MODE_PS3) return;
  BluetoothLock lock;
  bluetoothSync = enabled;
  if (currentMode == MODE_XBOX_BLE) { XboxBLE::sync(enabled); return; }
  // Accept Just Works confirmation only during the physical sync window.
  gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
  gap_ssp_set_auto_accept(enabled ? 1 : 0);
  gap_set_bondable_mode(enabled ? 1 : 0);
  gap_discoverable_control(enabled ? 1 : 0);
  gap_connectable_control(1); // Saved hosts can reach us outside discovery.
  if (enabled) pairingStatus = authenticationStatus = 0xff;
  if (currentMode == MODE_XBOX_CLASSIC) { if (enabled) XboxClassic::disconnect(); return; }
  if (enabled && PicoBluetoothHID.connected()) {
    hid_device_disconnect(PicoBluetoothHID.getCID());
  }
}

#include "BluetoothReconnect.h"

// Player slots belong to the host. Reconnect with the same identity and bonds
// so it can reassess this controller; it may assign the same slot again.
void requestHostReconnect() {
  if (currentMode == MODE_PS3) { PS3USB::reconnect(); return; }
  setBluetoothSync(false);
  BluetoothLock lock;
  if (currentMode == MODE_XBOX_BLE) {
    if (XboxBLE::connection != HCI_CON_HANDLE_INVALID) gap_disconnect(XboxBLE::connection);
    else XboxBLE::advertise();
  } else {
    btstack_linked_list_iterator_t connections;
    hci_connections_get_iterator(&connections);
    while (btstack_linked_list_iterator_has_next(&connections)) {
      auto *link = reinterpret_cast<hci_connection_t *>(btstack_linked_list_iterator_next(&connections));
      if (link->address_type == BD_ADDR_TYPE_ACL && link->con_handle != HCI_CON_HANDLE_INVALID)
        gap_disconnect(link->con_handle);
    }
    // Existing saved-host reconnect logic resumes after the link closes.
  }
  if (Serial) Serial.println("BT reconnect requested; player slot is host assigned");
}

#if BTPAD_HAS_DISPLAY
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool displayReady = false;
#endif

bool pressed(InputId id) {
  return stableState[id];
}

void setDefaultProfile(Profile &profile, const char *name) {
  strncpy(profile.name, name, sizeof(profile.name) - 1);
  profile.name[sizeof(profile.name) - 1] = '\0';
  for (uint8_t i = 0; i < IN_COUNT; i++) {
    profile.remap[i] = i;
  }
  profile.turboMask = 0;
  profile.turboHz = 12;
  profile.socd = SOCD_NEUTRAL;
}

void resetConfig() {
  memset(&config, 0, sizeof(config));
  config.magic = CONFIG_MAGIC;
  strcpy(config.adminPassword, "changeme");
  config.defaultMode = MODE_GAMEPAD;
  config.activeProfile = 0;
  setDefaultProfile(config.profiles[0], "Default");
  setDefaultProfile(config.profiles[1], "Alt 1");
  setDefaultProfile(config.profiles[2], "Alt 2");
}

bool saveConfig() {
  EEPROM.put(0, config);
  keyboardConfig.checksum = keyboardChecksum(keyboardConfig);
  EEPROM.put(sizeof(Config), keyboardConfig);
  return EEPROM.commit();
}

void loadConfig() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, config);
  EEPROM.get(sizeof(Config), keyboardConfig);
  if (!validKeyboardConfig(keyboardConfig)) keyboardConfig = defaultKeyboardConfig();
  if (!isConfigValid(config)) {
    resetConfig();
    saveConfig();
  } else {
    HidMode mode = migrateLegacyMode(config.defaultMode);
    if (mode != config.defaultMode) {
      config.defaultMode = mode;
      saveConfig();
    }
  }
}

bool pinHeld(uint8_t gpio) {
  pinMode(gpio, INPUT_PULLUP);
  delayMicroseconds(80);
  return digitalRead(gpio) == LOW;
}

HidMode detectBootMode() {
  return selectBootMode(config.defaultMode, pinHeld);
}

void initInputs() {
  for (const PinDef &pin : PIN_MAP) {
    pinMode(pin.gpio, INPUT_PULLUP);
    rawState[pin.id] = stableState[pin.id] = false;
    lastChangeMs[pin.id] = millis();
  }
}

void scanInputs() {
  uint32_t now = millis();
  for (const PinDef &pin : PIN_MAP) {
    bool reading = digitalRead(pin.gpio) == LOW;
    if (reading != rawState[pin.id]) {
      rawState[pin.id] = reading;
      lastChangeMs[pin.id] = now;
    }
    if ((now - lastChangeMs[pin.id]) >= DEBOUNCE_MS && stableState[pin.id] != reading) {

      stableState[pin.id] = reading;

    }
  }
}

bool logicalPressed(InputId logical) {
  return (logicalState & (1u << logical)) != 0;
}

uint32_t buildButtons() {
  return encodeGamepad(logicalState, currentMode).buttons;
}

void sendGamepadReport() {
  const GamepadOutput out = encodeGamepad(logicalState, currentMode);
  JoystickBT.X(out.x);
  JoystickBT.Y(out.y);
  JoystickBT.Z(out.z);
  JoystickBT.Zrotate(out.rz);
  JoystickBT.hat(out.hatAngle);
  for (uint8_t i = 0; i < 14; i++) {
    JoystickBT.button(i + 1, (out.buttons & (1u << i)) != 0);
  }
  JoystickBT.send_now();
}
void sendKeyboardReport() {
  static KeyboardReport report;
  report = encodeKeyboard(logicalState, keyboardConfig.maps[config.activeProfile]);
  PicoBluetoothHID.send(1, &report, sizeof(report));
}
#include "WebPortal.h"

void initBluetooth() {
  BluetoothLock lock;
  if (currentMode == MODE_XBOX_BLE) { XboxBLE::begin(); return; }
  pairingEvents.callback = observePairing;
  hci_add_event_handler(&pairingEvents);
  if (currentMode == MODE_XBOX_CLASSIC) { XboxClassic::begin(); return; }
  const char *name = bluetoothNames[currentMode];
  if (currentMode == MODE_KEYBOARD) {
    PicoBluetoothHID.startHID(name, name, 0x2540, 33, keyboardDescriptor, sizeof(keyboardDescriptor));
  } else {
    JoystickBT.HID_Joystick::begin();
    JoystickBT.use16bit();
    JoystickBT.useManualSend(true);
    PicoBluetoothHID.startHID(name, name, 0x2508, 33, gamepadDescriptor, sizeof(gamepadDescriptor));
  }
  // setup() applies initial sync state after releasing this stack lock.
}

void reportBluetoothStatus() {
  static uint32_t lastStatus = 0;
  if (!Serial || static_cast<uint32_t>(millis() - lastStatus) < 1000) return;
  lastStatus = millis();
  bool connected;
  uint8_t pairing, authentication;
  {
    BluetoothLock lock;
    connected = bluetoothConnected();
    pairing = pairingStatus;
    authentication = authenticationStatus;
  }
  Serial.printf("BT sync=%u connected=%u pairing=0x%02x auth=0x%02x\n",
                bluetoothSync, connected, pairing, authentication);
}

void updateTurboClock() {
  uint32_t now = millis();
  uint8_t turboHz = config.profiles[config.activeProfile].turboHz < 2 ? 2 : config.profiles[config.activeProfile].turboHz;
  uint16_t halfPeriod = 500 / turboHz;
  if ((now - lastTurboFlipMs) >= halfPeriod) {
    turboPhase = !turboPhase;
    lastTurboFlipMs = now;
  }
}

void updateDisplay() {
#if BTPAD_HAS_DISPLAY
  static uint32_t lastFrame = 0;
  const uint32_t now = millis();
  if (!displayReady || static_cast<uint32_t>(now - lastFrame) < 100) return;
  lastFrame = now;
  uint32_t physical = 0;
  for (uint8_t i = 0; i < IN_COUNT; ++i) if (stableState[i]) physical |= 1u << i;
  bool connected = false;
  if (currentMode != MODE_WEB) {
    BluetoothLock lock;
    connected = bluetoothConnected();
  }
  // Current Bluetooth descriptors have no host player-number feedback.
  const uint8_t player = currentMode == MODE_PS3 ? PS3USB::playerNumber() : 0;
  drawControllerDisplay(display, config, currentMode, physical, bluetoothSync, connected, player);
  display.display();
#endif
}
void setupDisplay() {
#if BTPAD_HAS_DISPLAY
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  Wire.beginTransmission(0x3C);
  displayReady = Wire.endTransmission() == 0 && display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (displayReady) {
    display.clearDisplay();
    display.display();
  }
#endif
}

void setup() {
  checkFirmwareBoot();
  Serial.begin(115200);
  loadConfig();
  initInputs();
  delay(40);
  currentMode = detectBootMode();
  // One-shot USB request survives watchdog reset, never an EEPROM setting.
  const bool usbWeb = watchdog_caused_reboot() && watchdog_hw->scratch[0] == 0x42545745;
  watchdog_hw->scratch[0] = 0;
  if (usbWeb) currentMode = MODE_WEB;
  setupDisplay();

  if (currentMode == MODE_WEB) {
    startWebConfig();
  } else if (currentMode == MODE_PS3) {
    PS3USB::begin();
  } else {
    initBluetooth();
    setBluetoothSync(false);
  }
}

void loop() {
  static UsbSetupCommand usbCommand;
  // Bound work per loop so a noisy serial sender cannot starve game reports.
  for (uint8_t n = 0; n < 32 && Serial.available(); ++n) {
    if (usbCommand.feed(static_cast<char>(Serial.read()))) {
      if (currentMode == MODE_WEB) {
        Serial.println("Web setup active: BTPad-Setup at http://192.168.4.1/");
        continue;
      }
      Serial.println("Opening BTPad-Setup at http://192.168.4.1/");
      Serial.flush();
      watchdog_hw->scratch[0] = 0x42545745;
      watchdog_reboot(0, 0, 50);
      while (true) delay(1);
    }
  }
  scanInputs();
  if (currentMode == MODE_WEB) {
    server.handleClient();
    updateDisplay();
    delay(2);
    return;
  }

  if (syncButton.update(pressed(IN_GUIDE), millis(),
                        pressed(IN_SPECIAL) || inputPipeline.suppressed(IN_GUIDE))) {
    setBluetoothSync(!bluetoothSync);
  }
  bool reportDue = static_cast<uint32_t>(millis() - lastReportMs) >= REPORT_MS;
  if (reportDue) syncTap = syncButton.takeTap();
  if (inputPipeline.scan(stableState, config, syncTap, currentMode == MODE_KEYBOARD, reportDue)) saveConfig();
  if (inputPipeline.takeReconnect()) requestHostReconnect();
  updateTurboClock();
  logicalState = inputPipeline.report(config.profiles[config.activeProfile], turboPhase);
  if (reportDue) {
    lastReportMs = millis();
    if (currentMode == MODE_PS3) PS3USB::send(logicalState);
    else if (currentMode == MODE_XBOX_BLE) XboxBLE::send(logicalState);
    else if (currentMode == MODE_XBOX_CLASSIC) XboxClassic::send(logicalState);
    else if (currentMode == MODE_KEYBOARD) sendKeyboardReport();
    else sendGamepadReport();
  }
  reportBluetoothStatus();
  bool linked;
  { BluetoothLock lock; linked = bluetoothConnected(); }
  if (bluetoothSync && linked) setBluetoothSync(false);
  if (currentMode != MODE_XBOX_BLE && currentMode != MODE_PS3) reconnectBluetooth(linked);
  updateDisplay();
}
