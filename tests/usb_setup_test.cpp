#include <initializer_list>
#include "../btpad_pico/UsbSetupCommand.h"
#include <assert.h>
#include <stdio.h>
int main() {
  UsbSetupCommand parser;
  for (char c : {'w', 'e', 'b'}) assert(!parser.feed(c));
  assert(parser.feed('\r')); assert(!parser.feed('\n'));
  for (const char *bad : {"notweb\n", "web extra\n", "webxxxxxxxxxxxxxxxxxxxx\n", "\n"})
    for (const char *p = bad; *p; ++p) assert(!parser.feed(*p));
  parser.feed('w'); parser.feed('e'); parser.feed('b'); assert(parser.feed('\n'));
  puts("USB setup command: exact match, CRLF, overflow and recovery passed.");
}
