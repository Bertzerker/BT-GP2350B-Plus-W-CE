#include "../btpad_pico/SyncButton.h"
#include <assert.h>
#include <stdio.h>
int main() {
  SyncButton button;
  assert(!button.update(false, 0));
  assert(!button.update(true, 10));
  assert(!button.takeTap());
  assert(!button.update(true, 3009));
  assert(button.update(true, 3010));
  assert(!button.update(true, 9000));
  assert(!button.update(false, 9001));
  assert(!button.takeTap());
  assert(!button.update(true, 10000));
  assert(!button.update(false, 10100));
  assert(button.takeTap());
  assert(!button.takeTap());
  assert(!button.update(true, 11000));
  assert(button.update(true, 14000));
  assert(!button.update(false, 14001));
  assert(!button.takeTap());
  SyncButton rollover;
  uint32_t start = UINT32_MAX - 999;
  assert(!rollover.update(true, start));
  assert(!rollover.update(true, start + 2999));
  assert(rollover.update(true, start + 3000));
  assert(!rollover.update(false, start + 3001));
  assert(!rollover.takeTap());
  SyncButton suppressed;
  assert(!suppressed.update(true, 0));
  assert(!suppressed.update(true, 1000, true));
  assert(!suppressed.update(true, 4000));
  assert(!suppressed.update(false, 5000));
  assert(!suppressed.takeTap());
  assert(!suppressed.update(true, 6000));
  assert(!suppressed.update(false, 6100));
  assert(suppressed.takeTap());
  puts("GP21 sync hold tests passed.");
}
