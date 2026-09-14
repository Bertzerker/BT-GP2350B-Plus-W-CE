/*
  Bluetooth HID controller for Waveshare RP2350B-Plus-W.

  Target: Earle Philhower arduino-pico core with IP/Bluetooth Stack enabled.
  This sketch uses the core's Bluetooth Classic HID helpers:
    - JoystickBT for gamepad-like modes
    - KeyboardBT for keyboard mode

  Notes:
    - Generic gamepad and keyboard are implemented. Native XInput, Switch
      and PS3 remain planned. True
      console-native Bluetooth compatibility requires protocol-specific report
      descriptors, pairing behavior, and in some cases authentication.
    - GP06 is listed twice in btpad.md for boot mode selection. This sketch
      uses GP06 for generic gamepad and offers DirectInput in the web UI.
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <JoystickBT.h>
#include <KeyboardBT.h>
#include <HID_Bluetooth.h>
#include <pico/bootrom.h>

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

const uint8_t KEY_LABELS[IN_COUNT] = {
  KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW, KEY_LEFT_ARROW,
  'z', 'x', 'a', 's', 'v', 'f', 'c', 'd',
  KEY_BACKSPACE, KEY_RETURN, 'b', 'g', 0, KEY_ESC, 0
};

constexpr size_t EEPROM_SIZE = sizeof(Config);
constexpr uint16_t DEBOUNCE_MS = 5;
constexpr uint16_t REPORT_MS = 4;

Config config;
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
  BluetoothLock lock;
  bluetoothSync = enabled;
  // Accept Just Works confirmation only during the physical sync window.
  gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
  gap_ssp_set_auto_accept(enabled ? 1 : 0);
  gap_set_bondable_mode(enabled ? 1 : 0);
  gap_discoverable_control(enabled ? 1 : 0);
  gap_connectable_control(1); // Saved hosts can reach us outside discovery.
  if (enabled) pairingStatus = authenticationStatus = 0xff;
  if (enabled && PicoBluetoothHID.connected()) {
    hid_device_disconnect(PicoBluetoothHID.getCID());
  }
}

#include "BluetoothReconnect.h"

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
  return EEPROM.commit();
}

void loadConfig() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, config);
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
  if (pinHeld(14)) return MODE_WEB;
  if (pinHeld(6)) return MODE_GAMEPAD;



  if (pinHeld(9)) return MODE_KEYBOARD;
  return config.defaultMode;
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
  static bool keyOutputState[IN_COUNT] = {};
  // Release old directions/keys before pressing replacements. KeyboardBT emits
  // a report on each call, so the reverse order can briefly send both opposites.
  for (uint8_t logical = 0; logical < IN_COUNT; ++logical) {
    if (KEY_LABELS[logical] && keyOutputState[logical] && !logicalPressed((InputId)logical)) {
      KeyboardBT.release(KEY_LABELS[logical]);
    }
  }
  for (uint8_t logical = 0; logical < IN_COUNT; ++logical) {
    if (!KEY_LABELS[logical]) continue;
    bool down = logicalPressed((InputId)logical);
    if (down && !keyOutputState[logical]) KeyboardBT.press(KEY_LABELS[logical]);
    keyOutputState[logical] = down;
  }
}
#include "WebPortal.h"

void initBluetooth() {
  BluetoothLock lock;
  pairingEvents.callback = observePairing;
  hci_add_event_handler(&pairingEvents);
  const char *name = bluetoothNames[currentMode];
  if (currentMode == MODE_KEYBOARD) {
    KeyboardBT.HID_Keyboard::begin();
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
    connected = PicoBluetoothHID.connected();
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
    connected = PicoBluetoothHID.connected();
  }
  drawControllerDisplay(display, config, currentMode, physical, bluetoothSync, connected);
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
  setupDisplay();

  if (currentMode == MODE_WEB) {
    startWebConfig();
  } else {
    initBluetooth();
    setBluetoothSync(false);
  }
}

void loop() {
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
  if (inputPipeline.scan(stableState, config, syncTap)) saveConfig();
  updateTurboClock();
  logicalState = inputPipeline.report(config.profiles[config.activeProfile], turboPhase);
  if (reportDue) {
    lastReportMs = millis();
    if (currentMode == MODE_KEYBOARD) sendKeyboardReport();
    else sendGamepadReport();
  }
  reportBluetoothStatus();
  reconnectBluetooth();
  updateDisplay();
}
