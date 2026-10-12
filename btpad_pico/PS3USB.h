#pragma once
#include <stdint.h>
namespace PS3USB {
void begin();
void reconnect();
bool connected();
uint8_t playerNumber();
void send(uint32_t logical);
}
