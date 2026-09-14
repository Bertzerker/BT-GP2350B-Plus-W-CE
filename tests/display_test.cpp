#include <assert.h>
#include <stdio.h>
#include <string>
#include "../btpad_pico/DisplayUI.h"
struct Screen {
  int cursorX = 0, cursorY = 0, fills = 0;
  std::string text;
  void clearDisplay() { text.clear(); fills = 0; }
  void setTextSize(int size) { assert(size == 1); }
  void setTextWrap(bool wrap) { assert(!wrap); }
  void setTextColor(int color) { assert(color == 0 || color == 1); }
  void setCursor(int x, int y) { cursorX = x; cursorY = y; }
  void print(char c) { assert(cursorX >= 0 && cursorX + 6 <= 128 && cursorY >= 0 && cursorY + 8 <= 64); cursorX += 6; text += c; }
  void print(const char *s) { while (*s) print(*s++); }
  void print(int n) { print(std::to_string(n).c_str()); }
  void drawRect(int x, int y, int w, int h, int) { assert(x >= 0 && y >= 0 && x+w <=128 && y+h<=64); }
  void fillRect(int x,int y,int w,int h,int c) { drawRect(x,y,w,h,c); ++fills; }
  void drawCircle(int x,int y,int r,int) { assert(x-r>=0 && y-r>=0 && x+r<128 && y+r<64); }
  void fillCircle(int x,int y,int r,int c) { drawCircle(x,y,r,c); ++fills; }
};
int main() {
  Config settings = {};
  auto &p = settings.profiles[0];
  for (int i=0;i<19;++i) p.name[i]='W';
  p.turboHz=30; p.turboMask=0xfff0;
  Screen screen;
  for (auto mode : {MODE_GAMEPAD, MODE_DIRECTINPUT, MODE_KEYBOARD, MODE_WEB}) {
    for (int socd=0;socd<3;++socd) {
      p.socd=static_cast<SocdMode>(socd);
      drawControllerDisplay(screen, settings, mode, (1u<<IN_COUNT)-1, true, false);
      if (mode != MODE_WEB) { assert(screen.fills==19); assert(screen.text.find("BT:SYNC")!=std::string::npos); }
      else assert(screen.text.find("192.168.4.1")!=std::string::npos);
    }
  }
  drawControllerDisplay(screen, settings, MODE_GAMEPAD, 0, false, true);
  assert(screen.fills==0 && screen.text.find("BT:ON")!=std::string::npos);
  puts("OLED layout passed: bounds, 19 held inputs, modes, turbo and status.");
}
