#pragma once
#include <stdint.h>
namespace PS3USB {
void begin();
bool connected();
void send(uint32_t logical);
}
