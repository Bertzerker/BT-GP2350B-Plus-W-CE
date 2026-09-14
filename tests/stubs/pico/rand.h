#pragma once
#include <stdint.h>
// Deterministic entropy only for desktop tests. Firmware uses the Pico SDK.
inline uint32_t get_rand_32() { static uint32_t value = 0; return ++value; }
