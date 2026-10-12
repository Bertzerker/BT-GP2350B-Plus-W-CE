#pragma once
#include <string.h>

// Bounded line parser for the physical USB serial port. No network endpoint.
class UsbSetupCommand {
public:
  bool feed(char c) {
    if (c == '\r' || c == '\n') {
      const bool web = !overflow && strcmp(line, "web") == 0;
      length = 0; overflow = false; line[0] = 0;
      return web;
    }
    if (length < sizeof(line) - 1) { line[length++] = c; line[length] = 0; }
    else overflow = true;
    return false;
  }
private:
  char line[16] = {};
  unsigned length = 0;
  bool overflow = false;
};
