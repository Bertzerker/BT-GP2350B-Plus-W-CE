#include "../btpad_pico/GamepadOutput.h"
#include <assert.h>
#include <stdio.h>

int main() {
  const uint32_t directions[] = {0, 1u << IN_UP, (1u << IN_UP) | (1u << IN_RIGHT),
    1u << IN_RIGHT, (1u << IN_RIGHT) | (1u << IN_DOWN), 1u << IN_DOWN,
    (1u << IN_DOWN) | (1u << IN_LEFT), 1u << IN_LEFT, (1u << IN_LEFT) | (1u << IN_UP)};
  const int angles[] = {-1, 0, 45, 90, 135, 180, 225, 270, 315};
  for (int i = 0; i < 9; ++i) {
    const auto out = encodeGamepad(directions[i], MODE_GAMEPAD);
    assert(out.hatAngle == angles[i] && out.x == 0 && out.y == 0);
  }
  for (unsigned int triggers = 0; triggers < 4; ++triggers) {
    const uint32_t state = ((triggers & 1) ? 1u << IN_LT : 0) | ((triggers & 2) ? 1u << IN_RT : 0);
    const auto out = encodeGamepad(state, MODE_GAMEPAD);
    assert(out.buttons == 0);
    assert(out.z == ((triggers & 1) ? 32767 : -32767));
    assert(out.rz == ((triggers & 2) ? 32767 : -32767));
    const auto legacy = encodeGamepad(state | (1u << IN_LEFT), MODE_DIRECTINPUT);
    assert(legacy.hatAngle == -1 && legacy.x == -32767 && legacy.y == 0);
    assert(legacy.buttons == (triggers << 6));
    assert(legacy.z == out.z && legacy.rz == out.rz);
  }
  assert(encodeGamepad(1u << IN_GUIDE, MODE_GAMEPAD).buttons == (1u << 12));
  puts("Gamepad reports passed: hat directions, trigger isolation, legacy DirectInput.");
}
