#include <assert.h>
#include <stdio.h>
constexpr int INPUT_PULLUP = 2, LOW = 0;
int readings[2], index = 0, resets = 0, waited = 0;
void pinMode(int pin, int mode) { assert(pin == 20 && mode == INPUT_PULLUP); }
void delay(int ms) { waited += ms; }
int digitalRead(int pin) { assert(pin == 20 && index < 2); return readings[index++]; }
void reset_usb_boot(unsigned int led, unsigned int disabled) { assert(!led && !disabled); ++resets; }
#include "../btpad_pico/BootControl.h"
int main() {
  for (int first = 0; first < 2; ++first) for (int second = 0; second < 2; ++second) {
    readings[0] = first; readings[1] = second; index = resets = waited = 0;
    checkFirmwareBoot();
    assert(resets == (first == LOW && second == LOW));
    assert(waited == (first == LOW ? 40 : 10));
  }
  puts("GP20 boot selector passed: stable hold, release, unpressed.");
}
