#pragma once
#include <ble/att_db_util.h>
#include <ble/att_server.h>
#include <ble/sm.h>
#include "XboxDescriptor.h"
#include "XboxReport.h"

// Experimental Xbox Series-compatible HOGP service. Uses its own ATT database,
// not the core's generic JoystickBLE descriptor or Classic SDP buffer.
namespace XboxBLE {
static hci_con_handle_t connection = HCI_CON_HANDLE_INVALID;
static uint16_t inputHandle, outputHandle, protocolHandle, controlHandle, mapHandle, serviceChangedHandle;
static bool subscribed = false, encrypted = false;
static uint32_t connectedAt = 0;
static uint8_t lastPairingStatus = 0xff, lastDisconnectReason = 0;
static uint8_t protocol = 1, control = 1;
static uint8_t ccc[2] = {}, serviceChangedCcc[2] = {}, rumble[8] = {};
static XboxReport report = encodeXbox(0);
static btstack_packet_callback_registration_t hciEvents, smEvents;
static uint8_t advertisements[] = {2,1,6, 3,3,0x12,0x18, 3,0x19,0xc4,0x03,
  11,9,'B','T','P','a','d','-','X','b','o','x'};

inline bool connected() { return connection != HCI_CON_HANDLE_INVALID && gap_encryption_key_size(connection) > 0 && subscribed; }
inline void advertise() {
  advertisements[2] = bluetoothSync ? 6 : 4;
  gap_advertisements_set_data(sizeof(advertisements), advertisements);
  if (connection == HCI_CON_HANDLE_INVALID) gap_advertisements_enable(1);
}
inline void sync(bool enabled) {
  if (enabled && connection != HCI_CON_HANDLE_INVALID) gap_disconnect(connection);
  advertise();
}
static uint16_t read(hci_con_handle_t, uint16_t handle, uint16_t offset, uint8_t *buffer, uint16_t size) {
  if (handle == mapHandle) {
    if (Serial) Serial.printf("BLE report map offset=%u requested=%u total=%u\n", offset, size, unsigned(sizeof(XboxOneS_1914_HIDDescriptor)));
    return att_read_callback_handle_blob(XboxOneS_1914_HIDDescriptor, sizeof(XboxOneS_1914_HIDDescriptor), offset, buffer, size);
  }
  if (handle == inputHandle) return att_read_callback_handle_blob(report.bytes, sizeof(report.bytes), offset, buffer, size);
  if (handle == serviceChangedHandle + 1) return att_read_callback_handle_blob(serviceChangedCcc, 2, offset, buffer, size);
  if (handle == inputHandle + 1) return att_read_callback_handle_blob(ccc, 2, offset, buffer, size);
  if (handle == outputHandle) return att_read_callback_handle_blob(rumble, 8, offset, buffer, size);
  if (handle == protocolHandle) return att_read_callback_handle_blob(&protocol, 1, offset, buffer, size);
  return 0;
}
static int write(hci_con_handle_t, uint16_t handle, uint16_t transaction, uint16_t offset, uint8_t *buffer, uint16_t size) {
  if (transaction != ATT_TRANSACTION_MODE_NONE || offset != 0) return ATT_ERROR_REQUEST_NOT_SUPPORTED;
  if (handle == serviceChangedHandle + 1 && size == 2) {
    if (buffer[1] || (buffer[0] != 0 && buffer[0] != 2)) return ATT_ERROR_VALUE_NOT_ALLOWED;
    memcpy(serviceChangedCcc, buffer, 2); return 0;
  }
  if (handle == inputHandle + 1 && size == 2) {
    if (buffer[1] || buffer[0] > 1) return ATT_ERROR_VALUE_NOT_ALLOWED;
    memcpy(ccc, buffer, 2); subscribed = buffer[0] == 1;
    if (Serial) Serial.printf("BLE input subscription=%u\n", subscribed);
    return 0;
  }
  if (handle == outputHandle && size == 8) { memcpy(rumble, buffer, 8); return 0; } // No motors fitted.
  if (handle == protocolHandle && size == 1 && buffer[0] == 1) { protocol = 1; return 0; }
  if (handle == controlHandle && size == 1 && buffer[0] <= 1) { control = buffer[0]; return 0; }
  return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
}
static void event(uint8_t type, uint16_t, uint8_t *packet, uint16_t size) {
  if (type != HCI_EVENT_PACKET || size < 3) return;
  switch (hci_event_packet_get_type(packet)) {
    case HCI_EVENT_META_GAP:
      if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE &&
          gap_subevent_le_connection_complete_get_status(packet) == 0) {
        connection = gap_subevent_le_connection_complete_get_connection_handle(packet);
        connectedAt = millis();
        if (Serial) Serial.println("BLE link established; waiting for encryption and HID subscription");
        subscribed = encrypted = false; control = 1; memset(ccc, 0, 2);
        sm_request_pairing(connection); // Reuses an existing bond, or requests GP20-authorized pairing.
      }
      break;
    case HCI_EVENT_ENCRYPTION_CHANGE:
      if (hci_event_encryption_change_get_connection_handle(packet) == connection)
        encrypted = hci_event_encryption_change_get_encryption_enabled(packet) != 0;
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      if (hci_event_disconnection_complete_get_connection_handle(packet) == connection) {
        lastDisconnectReason = hci_event_disconnection_complete_get_reason(packet);
        if (Serial) Serial.printf("BLE disconnected reason=0x%02x\n", lastDisconnectReason);
        connection = HCI_CON_HANDLE_INVALID; subscribed = encrypted = false; memset(ccc, 0, 2); advertise();
      }
      break;
    case SM_EVENT_PAIRING_COMPLETE:
      lastPairingStatus = sm_event_pairing_complete_get_status(packet);
      if (Serial) Serial.printf("BLE pairing status=0x%02x reason=0x%02x\n", lastPairingStatus, sm_event_pairing_complete_get_reason(packet));
      break;
    case SM_EVENT_JUST_WORKS_REQUEST: {
      const auto handle = sm_event_just_works_request_get_handle(packet);
      if (bluetoothSync) sm_just_works_confirm(handle); else { sm_bonding_decline(handle); gap_disconnect(handle); }
      break;
    }
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST: {
      const auto handle = sm_event_numeric_comparison_request_get_handle(packet);
      if (bluetoothSync) sm_numeric_comparison_confirm(handle); else { sm_bonding_decline(handle); gap_disconnect(handle); }
      break;
    }
  }
}
inline uint16_t characteristic(uint16_t uuid, uint16_t properties, const void *value, uint16_t length, uint8_t security = ATT_SECURITY_NONE) {
  return att_db_util_add_characteristic_uuid16(uuid, properties, security, security,
    const_cast<uint8_t *>(static_cast<const uint8_t *>(value)), length);
}
inline void begin() {
  l2cap_init(); sm_init();
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
  sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
  att_db_util_init();
  // Generic Attribute service is required alongside Generic Access for HOGP.
  // This firmware uses a fixed database within a build. Re-pair after updates.
  att_db_util_add_service_uuid16(0x1801);
  static const uint8_t fullDatabase[] = {1,0,0xff,0xff};
  serviceChangedHandle = characteristic(0x2a05, ATT_PROPERTY_INDICATE, fullDatabase, sizeof(fullDatabase));
  att_db_util_add_service_uuid16(0x1800);
  const char name[] = "BTPad-Xbox";
  characteristic(0x2a00, ATT_PROPERTY_READ, name, sizeof(name)-1);
  const uint8_t appearance[] = {0xc4,3}; characteristic(0x2a01, ATT_PROPERTY_READ, appearance, 2);
  att_db_util_add_service_uuid16(0x180a);
  const uint8_t pnp[] = {2,0x5e,0x04,0x13,0x0b,0x0a,0x05}; // USB VID/PID for the emulated Xbox BLE report family.
  characteristic(0x2a50, ATT_PROPERTY_READ, pnp, sizeof(pnp));
  const char maker[] = "BTPad experimental"; characteristic(0x2a29, ATT_PROPERTY_READ, maker, sizeof(maker)-1);
  att_db_util_add_service_uuid16(0x180f);
  const uint8_t battery = 100; characteristic(0x2a19, ATT_PROPERTY_READ, &battery, 1);
  att_db_util_add_service_uuid16(0x1812);
  const uint8_t info[] = {0x11,1,0,2}; characteristic(0x2a4a, ATT_PROPERTY_READ, info, 4);
  mapHandle = characteristic(0x2a4b, ATT_PROPERTY_READ | ATT_PROPERTY_DYNAMIC, nullptr, 0, ATT_SECURITY_ENCRYPTED);
  protocolHandle = characteristic(0x2a4e, ATT_PROPERTY_READ | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE | ATT_PROPERTY_DYNAMIC, nullptr, 0, ATT_SECURITY_ENCRYPTED);
  controlHandle = characteristic(0x2a4c, ATT_PROPERTY_WRITE_WITHOUT_RESPONSE | ATT_PROPERTY_DYNAMIC, nullptr, 0, ATT_SECURITY_ENCRYPTED);
  inputHandle = characteristic(0x2a4d, ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY | ATT_PROPERTY_DYNAMIC, nullptr, 0, ATT_SECURITY_ENCRYPTED);
  uint8_t ref[] = {1,1}; att_db_util_add_descriptor_uuid16(0x2908, ATT_PROPERTY_READ, 0, 0, ref, 2);
  outputHandle = characteristic(0x2a4d, ATT_PROPERTY_READ | ATT_PROPERTY_WRITE | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE | ATT_PROPERTY_DYNAMIC, nullptr, 0, ATT_SECURITY_ENCRYPTED);
  ref[0]=3; ref[1]=2; att_db_util_add_descriptor_uuid16(0x2908, ATT_PROPERTY_READ, 0, 0, ref, 2);
  att_server_init(att_db_util_get_address(), read, write);
  hciEvents.callback = event; hci_add_event_handler(&hciEvents);
  smEvents.callback = event; sm_add_event_handler(&smEvents);
  bd_addr_t empty = {};
  gap_advertisements_set_params(0x30, 0x60, 0, 0, empty, 7, 0);
  advertise(); hci_power_control(HCI_POWER_ON);
}
inline void send(uint32_t logical) {
  BluetoothLock lock;
  // Pairing is not a usable HID connection until the host enables reports.
  if (connection != HCI_CON_HANDLE_INVALID && !connected() &&
      static_cast<uint32_t>(millis() - connectedAt) > 30000) {
    if (Serial) Serial.printf("BLE setup stalled: key=%u subscribed=%u pairing=0x%02x\n", gap_encryption_key_size(connection), subscribed, lastPairingStatus);
    gap_disconnect(connection);
    connectedAt = millis();
  }
  static uint32_t pendingPresses = 0, previous = 0;
  if (!connected() || control != 1) {
    pendingPresses = previous = 0;
    report = encodeXbox(0);
    return;
  }
  // Preserve short Guide taps (and other press edges) while ATT is busy.
  pendingPresses |= logical & ~previous;
  previous = logical;
  report = encodeXbox(logical | pendingPresses);
  if (att_server_can_send_packet_now(connection) &&
      att_server_notify(connection, inputHandle, report.bytes, sizeof(report.bytes)) == ERROR_CODE_SUCCESS)
    pendingPresses = 0;
}
}
