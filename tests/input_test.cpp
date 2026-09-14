#include "../btpad_pico/InputPipeline.h"
#include <assert.h>
#include <stdio.h>

struct Rig {
  Config config = {};
  InputPipeline pipeline;
  bool keys[IN_COUNT] = {};
  Rig(SocdMode mode = SOCD_NEUTRAL) {
    for (Profile &p : config.profiles) {
      p.turboHz = 12; p.socd = mode;
      for (uint8_t i = 0; i < IN_COUNT; ++i) p.remap[i] = i;
    }
  }
  bool scan(bool capture = false) { return pipeline.scan(keys, config, capture); }
  uint32_t output(bool turbo = true) { return pipeline.report(config.profiles[config.activeProfile], turbo); }
  bool down(InputId id, bool turbo = true) { return (output(turbo) & (1u << id)) != 0; }
};

int main() {
  // All 16 directional combinations in all modes; same-scan opposites tie.
  for (int mode = 0; mode < 3; ++mode) {
    for (unsigned int bits = 0; bits < 16; ++bits) {
      Rig r(static_cast<SocdMode>(mode));
      for (int i = 0; i < 4; ++i) r.keys[i] = (bits & (1u << i)) != 0;
      r.scan();
      unsigned int expected = bits;
      if ((bits & 3) == 3) expected = (expected & ~3u) | (mode == SOCD_UP_PRIORITY ? 1u : 0u);
      if ((bits & 12) == 12) expected &= ~12u;
      assert(r.output() == expected);
    }
  }
  // Both axes and both press orders, including release/repress of winner.
  const InputId axes[][2] = {{IN_UP, IN_DOWN}, {IN_LEFT, IN_RIGHT}};
  for (const auto &axis : axes) for (int order = 0; order < 2; ++order) {
    Rig r(SOCD_LAST_INPUT);
    InputId first = axis[order], last = axis[1 - order];
    r.keys[first] = true; r.scan();
    r.keys[last] = true; r.scan();
    assert(r.down(last) && !r.down(first));
    r.keys[last] = false; r.scan();
    assert(r.down(first) && !r.down(last));
    r.keys[last] = true; r.scan();
    assert(r.down(last) && !r.down(first));
  }
  Rig remapped(SOCD_LAST_INPUT);
  remapped.config.profiles[0].remap[IN_A] = IN_UP;
  remapped.config.profiles[0].remap[IN_B] = IN_DOWN;
  remapped.keys[IN_B] = true; remapped.scan();
  remapped.keys[IN_A] = true; remapped.scan();
  assert(remapped.down(IN_UP) && !remapped.down(IN_DOWN));
  // A second physical source for an already-held direction is not a new edge.
  remapped.keys[IN_DOWN] = true; remapped.scan();
  assert(remapped.down(IN_UP));
  remapped.config.profiles[0].turboMask = 1u << IN_A;
  remapped.scan();
  assert(!remapped.down(IN_UP, false) && !remapped.down(IN_DOWN, false));
  assert(remapped.down(IN_UP, true));
  // A non-turbo source for the same logical action keeps it held.
  remapped.keys[IN_UP] = true; remapped.scan();
  assert(remapped.down(IN_UP, false));

  Rig profile;
  profile.keys[IN_SPECIAL] = profile.keys[IN_UP] = profile.keys[IN_A] = true;
  assert(profile.scan());
  assert(profile.config.activeProfile == 1 && profile.output() == 0);
  assert(!profile.scan() && profile.config.activeProfile == 1);
  profile.keys[IN_SPECIAL] = false; profile.scan();
  assert(profile.output() == 0); // Both chord inputs stay blocked.
  profile.keys[IN_SPECIAL] = true;
  assert(!profile.scan() && profile.config.activeProfile == 1); // No reused held chord.
  profile.keys[IN_SPECIAL] = profile.keys[IN_UP] = profile.keys[IN_A] = false; profile.scan();
  profile.keys[IN_A] = true; profile.scan();
  assert(profile.down(IN_A));

  Rig turbo;
  turbo.keys[IN_SPECIAL] = true; assert(!turbo.scan());
  turbo.keys[IN_X] = true; assert(turbo.scan());
  assert(turbo.config.profiles[0].turboMask == (1u << IN_X));
  assert(turbo.output() == 0);
  turbo.keys[IN_X] = false; turbo.scan();
  turbo.keys[IN_Y] = true; assert(!turbo.scan()); // One action per Special hold.
  turbo.keys[IN_SPECIAL] = false; turbo.scan();
  assert(!turbo.down(IN_Y));
  turbo.keys[IN_Y] = false; turbo.scan();
  turbo.keys[IN_Y] = true; turbo.scan();
  assert(turbo.down(IN_Y));
  // A key sent before Special was pressed is released when Special joins.
  turbo.keys[IN_SPECIAL] = true; turbo.scan();
  assert(!turbo.down(IN_Y));

  Rig bounds;
  bounds.config.profiles[0].turboHz = 2;
  bounds.keys[IN_SPECIAL] = bounds.keys[IN_LEFT] = bounds.keys[IN_B] = true;
  assert(!bounds.scan());
  assert(bounds.config.profiles[0].turboHz == 2 && bounds.config.profiles[0].turboMask == 0);
  assert(bounds.output() == 0); // A clamped speed chord must not toggle turbo instead.
  Rig sync;
  sync.keys[IN_GUIDE] = true;
  sync.scan(false); assert(!sync.down(IN_GUIDE)); // GP20 hold is withheld.
  sync.keys[IN_GUIDE] = false;
  sync.scan(true); assert(sync.down(IN_GUIDE));
  sync.scan(false); assert(!sync.down(IN_GUIDE));
  sync.keys[IN_SPECIAL] = true;
  sync.scan(true); assert(!sync.down(IN_GUIDE));
  Rig capture;
  capture.keys[IN_CAPTURE] = true;
  capture.scan(); assert(capture.down(IN_CAPTURE)); // GP21 is an ordinary held input again.
  capture.scan(); assert(capture.down(IN_CAPTURE));
  capture.keys[IN_CAPTURE] = false;
  capture.scan(); assert(!capture.down(IN_CAPTURE));

  Rig reset(SOCD_LAST_INPUT);
  reset.keys[IN_UP] = true; reset.scan();
  reset.keys[IN_DOWN] = true; reset.scan();
  assert(reset.down(IN_DOWN));
  reset.config.activeProfile = 1; reset.scan();
  assert(!reset.down(IN_UP) && !reset.down(IN_DOWN)); // No stale priority in new profile.
  puts("Shared input pipeline tests passed.");
}
