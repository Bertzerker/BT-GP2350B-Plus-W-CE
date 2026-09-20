#include <Arduino.h>
#include <USB.h>
#include <tusb.h>
#include <tusb-hid.h>
#include <pico/unique_id.h>
#include "PS3USB.h"
#include "PS3Report.h"

namespace PS3USB {
static bool active = false;
static PS3InputReport report = encodePS3(0);
static PS3Features features;
void begin() {
  USB.disconnect();
  Serial.end(); // PS3 test mode presents a controller, without CDC serial.
  pico_unique_board_id_t unique;
  pico_get_unique_board_id(&unique);
  memcpy(features.deviceAddress, unique.id, 6);
  features.deviceAddress[0] = (features.deviceAddress[0] | 2) & 0xfe;
  USB.setVIDPID(0x054c, 0x0268);
  USB.setManufacturer("BTPad");
  USB.setProduct("PLAYSTATION(R)3 Controller");
  USB.registerHIDDevice(ps3_report_descriptor, sizeof(ps3_report_descriptor), 10, 0);
  active = true;
  delay(250);
  USB.connect();
}
bool connected() { return active && tud_mounted(); }
void send(uint32_t logical) {
  static uint32_t pending = 0, previous = 0;
  if (!connected()) { pending = previous = 0; return; }
  pending |= logical & ~previous; previous = logical;
  mutex_enter_blocking(&USB.mutex);
  report = encodePS3(logical | pending);
  if (tud_hid_ready() && tud_hid_report(0, report.bytes, sizeof(report.bytes))) pending = 0;
  mutex_exit(&USB.mutex);
}
}
extern "C" uint16_t tud_hid_get_report_cb(uint8_t, uint8_t id, hid_report_type_t type, uint8_t *buffer, uint16_t length) {
  if (!PS3USB::active) return 0;
  if (type == HID_REPORT_TYPE_FEATURE) return PS3USB::features.get(id, buffer, length);
  if (type == HID_REPORT_TYPE_INPUT && id == 1) {
    // TinyUSB prefixes the requested report ID before invoking this callback.
    uint16_t count = length < 48 ? length : 48;
    memcpy(buffer, PS3USB::report.bytes + 1, count); return count;
  }
  return 0;
}
extern "C" void tud_hid_set_report_cb(uint8_t, uint8_t id, hid_report_type_t type, const uint8_t *buffer, uint16_t length) {
  if (PS3USB::active && type == HID_REPORT_TYPE_FEATURE) PS3USB::features.set(id, buffer, length);
  // Output (LED/rumble) is accepted; this hardware has no rumble motors.
}
