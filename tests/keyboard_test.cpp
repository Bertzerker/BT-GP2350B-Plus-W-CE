#include "../btpad_pico/KeyboardMap.h"
#include "../btpad_pico/InputPipeline.h"
#include <assert.h>
#include <stdio.h>
int main() {
  auto keys = defaultKeyboardConfig(); assert(validKeyboardConfig(keys));
  for (const auto &key : KEY_OPTIONS) {
    keys.maps[0][IN_SOUTH] = key.usage;
    const auto report = encodeKeyboard(1u << IN_SOUTH, keys.maps[0]);
    if (key.usage >= 224) assert(report.modifiers == (1u << (key.usage-224)));
    else assert(report.keys[0] == key.usage);
  }
  keys = defaultKeyboardConfig(); keys.maps[0][IN_SOUTH] = 224; keys.maps[0][IN_EAST] = 4;
  auto report = encodeKeyboard((1u<<IN_SOUTH)|(1u<<IN_EAST), keys.maps[0]);
  assert(report.modifiers == 1 && report.keys[0] == 4);
  keys.maps[0][IN_SOUTH] = 4;
  report = encodeKeyboard((1u<<IN_SOUTH)|(1u<<IN_EAST), keys.maps[0]);
  assert(report.keys[0]==4 && report.keys[1]==0); // Duplicate mapping releases only with last input.
  report = encodeKeyboard(0, keys.maps[0]); assert(report.keys[0]==0 && report.modifiers==0);
  for (uint8_t i=0;i<7;++i) keys.maps[0][i]=4+i;
  report = encodeKeyboard(0x7f, keys.maps[0]); for (auto key : report.keys) assert(key==1);
  keys = defaultKeyboardConfig(); keys.maps[1][IN_SOUTH]^=1; assert(!validKeyboardConfig(keys));
  Config config = {}; config.profiles[0].turboHz=12;
  for (uint8_t i=0;i<IN_COUNT;++i) config.profiles[0].remap[i]=i;
  config.profiles[0].remap[IN_SOUTH]=IN_EAST;
  InputPipeline pipeline; bool physical[IN_COUNT]={}; physical[IN_SOUTH]=true;
  pipeline.scan(physical,config,false); assert(pipeline.report(config.profiles[0],true)==(1u<<IN_EAST));
  pipeline.scan(physical,config,false,true); assert(pipeline.report(config.profiles[0],true)==(1u<<IN_SOUTH));
  puts("Keyboard mapping passed: all keys, modifiers, deduplication, rollover, storage integrity and protocol independence.");
}
