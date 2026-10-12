#pragma once

// Called under BluetoothLock. The stack's connection list includes pending ACL
// attempts: never reset its singleton HID state while a connection is in flight.
void reconnectBluetooth(bool linked) {
  static uint32_t lastAttempt = 0;
  static unsigned int nextHost = 0;
  static bool trackingBusy = false;
  static uint32_t busySince = 0;
  BluetoothLock lock;
  const uint32_t now = millis();
  if (bluetoothSync || linked || hci_get_state() != HCI_STATE_WORKING) {
    lastAttempt = now;
    trackingBusy = false;
    return;
  }
  if (static_cast<uint32_t>(now - lastAttempt) < 10000) return;
  btstack_linked_list_iterator_t connections;
  hci_connections_get_iterator(&connections);
  if (btstack_linked_list_iterator_has_next(&connections)) {
    if (!trackingBusy) { trackingBusy = true; busySince = now; }
    // An ACL without HID must not suppress reconnect forever. Leave genuine
    // pending paging to the controller's own timeout; close established stalls.
    if (static_cast<uint32_t>(now - busySince) >= 20000) {
      while (btstack_linked_list_iterator_has_next(&connections)) {
        auto *link = reinterpret_cast<hci_connection_t *>(btstack_linked_list_iterator_next(&connections));
        if (link->address_type == BD_ADDR_TYPE_ACL && link->con_handle != HCI_CON_HANDLE_INVALID) {
          gap_disconnect(link->con_handle);
          if (Serial) Serial.printf("BT reconnect stale ACL=%u\n", link->con_handle);
        }
      }
      busySince = now;
    }
    return;
  }
  trackingBusy = false;
  lastAttempt = now;

  btstack_link_key_iterator_t iterator;
  if (!gap_link_key_iterator_init(&iterator)) return;
  bd_addr_t address, selected;
  link_key_t key;
  link_key_type_t type;
  unsigned int count = 0;
  bool found = false;
  while (gap_link_key_iterator_get_next(&iterator, address, key, &type)) {
    if (count == 0 || count == nextHost) {
      memcpy(selected, address, sizeof(selected));
      found = true;
    }
    ++count;
  }
  gap_link_key_iterator_done(&iterator);
  if (Serial) Serial.printf("BT reconnect bonds=%u\n", count);
  if (!found) return; // Pairing remains a physical GP20 action.
  nextHost = (nextHost < count ? nextHost + 1 : 1) % count;
  uint16_t cid = 0;
  const uint8_t status = hid_device_connect(selected, &cid);
  if (Serial) Serial.printf("BT reconnect status=0x%02x\n", status);
}
