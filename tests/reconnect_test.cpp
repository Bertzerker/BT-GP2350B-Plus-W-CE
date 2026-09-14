#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
struct BluetoothLock { BluetoothLock() {} ~BluetoothLock() {} };
bool bluetoothSync = false, connected = false, busy = false;
uint32_t testNow = 0;
uint32_t millis() { return testNow; }
constexpr int HCI_STATE_WORKING = 1;
int hci_get_state() { return HCI_STATE_WORKING; }
struct { bool connected() { return ::connected; } } PicoBluetoothHID;
struct btstack_linked_list_iterator_t {};
void hci_connections_get_iterator(btstack_linked_list_iterator_t *) {}
bool btstack_linked_list_iterator_has_next(btstack_linked_list_iterator_t *) { return busy; }
typedef uint8_t bd_addr_t[6];
typedef uint8_t link_key_t[16];
typedef int link_key_type_t;
struct btstack_link_key_iterator_t { int index; };
int hosts = 2, calls = 0, testSelected = -1;
int gap_link_key_iterator_init(btstack_link_key_iterator_t *it) { it->index = 0; return 1; }
int gap_link_key_iterator_get_next(btstack_link_key_iterator_t *it, bd_addr_t addr, link_key_t, link_key_type_t *) {
  if (it->index >= hosts) return 0;
  memset(addr, 0, 6); addr[0] = static_cast<uint8_t>(++it->index); return 1;
}
void gap_link_key_iterator_done(btstack_link_key_iterator_t *) {}
uint8_t hid_device_connect(bd_addr_t addr, uint16_t *) { ++calls; testSelected = addr[0]; busy = true; return 0; }
struct {
  explicit operator bool() const { return false; }
  void printf(const char *, unsigned int) {}
} Serial;
#include "../btpad_pico/BluetoothReconnect.h"
int main() {
  reconnectBluetooth(); assert(calls == 0);
  testNow = 10000; reconnectBluetooth(); assert(calls == 1 && testSelected == 1);
  testNow += 60000; reconnectBluetooth(); assert(calls == 1); // Pending ACL is not overwritten.
  busy = false; reconnectBluetooth(); assert(calls == 2 && testSelected == 2);
  busy = false; bluetoothSync = true; testNow += 10000; reconnectBluetooth(); assert(calls == 2);
  bluetoothSync = false; testNow += 9999; reconnectBluetooth(); assert(calls == 2);
  ++testNow; reconnectBluetooth(); assert(calls == 3 && testSelected == 1);
  busy = false; connected = true; testNow += 10000; reconnectBluetooth(); assert(calls == 3);
  connected = false; hosts = 0; testNow += 10000; reconnectBluetooth(); assert(calls == 3);
  hosts = 1; testNow = UINT32_MAX - 1000; bluetoothSync = true; reconnectBluetooth();
  bluetoothSync = false; testNow += 10000; reconnectBluetooth(); assert(calls == 4 && testSelected == 1);
  puts("Reconnect scheduling passed: saved hosts, in-flight guard, sync, no bonds and clock rollover.");
}
