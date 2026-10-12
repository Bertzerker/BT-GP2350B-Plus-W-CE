#pragma once
#include "XboxClassicDescriptor.h"
#include "XboxClassicReport.h"

// A dedicated Classic HID device: the core helper has a small SDP buffer and
// hard-coded generic Device ID. All accesses run under BluetoothLock/BTstack.
namespace XboxClassic {
static uint16_t cid = 0;
static bool requested = false, suspended = false;
static uint8_t initialReport = 0;
static uint8_t hidRecord[1024], deviceRecord[100], rumble[8] = {};
static XboxClassicInput input;
static btstack_packet_callback_registration_t events;
static constexpr char name[] = "BTPad-Xbox-Classic";
static_assert(sizeof(xboxClassicDescriptor) + sizeof(name) + 256 < sizeof(hidRecord), "Classic SDP buffer too small");

inline bool connected() { return cid != 0; }
inline void disconnect() { if (cid) hid_device_disconnect(cid); }
static int getReport(uint16_t, hid_report_type_t_bt type, uint16_t id, int *size, uint8_t *out) {
  *size = 0;
  if (type == HID_REPORT_TYPE_INPUT_BT) {
    if (id == 1) { const auto report = input.current(); memcpy(out, report.bytes, 16); *size = 16; }
    else if (id == 2) { out[0] = 0; *size = 1; } // AC Home unused; Guide is button 13 in report 1.
    else if (id == 4) { out[0] = 255; *size = 1; } // Placeholder, no battery measurement.
  } else if (type == HID_REPORT_TYPE_OUTPUT_BT && id == 3) {
    memcpy(out, rumble, 8); *size = 8;
  }
  return *size ? 1 : -1;
}
static void setReport(uint16_t, hid_report_type_t_bt type, int size, uint8_t *data) {
  // Control SET_REPORT includes the report ID in BTstack.
  if (type == HID_REPORT_TYPE_OUTPUT_BT && size == 9 && data[0] == 3) memcpy(rumble, data + 1, 8);
}
static void reportData(uint16_t, hid_report_type_t_bt type, uint16_t id, int size, uint8_t *data) {
  // Interrupt DATA callback has the ID stripped by BTstack.
  if (type == HID_REPORT_TYPE_OUTPUT_BT && id == 3 && size == 8) memcpy(rumble, data, 8);
}
static void event(uint8_t type, uint16_t, uint8_t *packet, uint16_t size) {
  if (type != HCI_EVENT_PACKET || size < 3 || hci_event_packet_get_type(packet) != HCI_EVENT_HID_META) return;
  switch (hci_event_hid_meta_get_subevent_code(packet)) {
    case HID_SUBEVENT_CONNECTION_OPENED: {
      const uint8_t status = hid_subevent_connection_opened_get_status(packet);
      cid = status == ERROR_CODE_SUCCESS ? hid_subevent_connection_opened_get_hid_cid(packet) : 0;
      input.clear(); requested = suspended = false; initialReport = 2;
      if (Serial) Serial.printf("Classic HID open status=0x%02x cid=%u\n", status, cid);
      break;
    }
    case HID_SUBEVENT_CONNECTION_CLOSED:
      if (hid_subevent_connection_closed_get_hid_cid(packet) != cid) break;
      cid = 0; input.clear(); requested = suspended = false;
      if (Serial) Serial.println("Classic HID closed");
      break;
    case HID_SUBEVENT_SUSPEND:
      suspended = true; input.clear(); break;
    case HID_SUBEVENT_EXIT_SUSPEND:
      suspended = false; break;
    case HID_SUBEVENT_VIRTUAL_CABLE_UNPLUG:
      disconnect(); break;
    case HID_SUBEVENT_CAN_SEND_NOW: {
      if (!cid || hid_subevent_can_send_now_get_hid_cid(packet) != cid) break;
      requested = false;
      if (suspended) break;
      uint8_t data[18] = {0xa1, 1};
      uint16_t length = sizeof(data);
      if (initialReport) {
        data[1] = initialReport == 2 ? 2 : 4;
        data[2] = initialReport == 2 ? 0 : 255;
        length = 3; --initialReport;
      } else {
        const auto report = input.current(); memcpy(data + 2, report.bytes, 16);
        input.sent();
      }
      hid_device_send_interrupt_message(cid, data, length);
      break;
    }
  }
}
inline void begin() {
  l2cap_init(); sm_init(); sdp_init();
  gap_set_local_name(name);
  gap_set_class_of_device(0x2508);
  gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE);
  gap_set_allow_role_switch(true);
  gap_set_security_level(LEVEL_2);
  // HID gamepad subclass, no country code; normally connectable with reconnect.
  const hid_sdp_record_t params = {0x08, 0, 1, 1, 1, true, false, 1600, 3200, 3200,
    xboxClassicDescriptor, sizeof(xboxClassicDescriptor), name};
  hid_create_sdp_record(hidRecord, 0x10001, &params);
  sdp_register_service(hidRecord);
  device_id_create_sdp_record(deviceRecord, 0x10003, DEVICE_ID_VENDOR_ID_SOURCE_USB, 0x045e, 0x02e0, 0x0903);
  sdp_register_service(deviceRecord);
  hid_device_init(false, sizeof(xboxClassicDescriptor), xboxClassicDescriptor);
  hid_device_register_packet_handler(event);
  hid_device_register_report_request_callback(getReport);
  hid_device_register_set_report_callback(setReport);
  hid_device_register_report_data_callback(reportData);
  events.callback = event; hci_add_event_handler(&events);
  hci_power_control(HCI_POWER_ON);
}
inline void send(uint32_t logical) {
  BluetoothLock lock;
  if (!connected() || suspended) { input.clear(); return; }
  input.update(logical);
  if (!requested) { requested = true; hid_device_request_can_send_now_event(cid); }
}
}
