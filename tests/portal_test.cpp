#include <algorithm>
#include <assert.h>
#include <functional>
#include <map>
#include <stdio.h>
#include <string>
#include <string.h>
#include "../btpad_pico/BoardInputs.h"
#include "../btpad_pico/KeyboardMap.h"

// Transport doubles execute the real route handlers without a radio or flash.
class String {
public:
  std::string value;
  String() = default;
  String(const char *text) : value(text) {}
  String(const std::string &text) : value(text) {}
  String(int number) : value(std::to_string(number)) {}
  unsigned int length() const { return static_cast<unsigned int>(value.size()); }
  char operator[](unsigned int i) const { return value[i]; }
  int indexOf(char character, int start) const {
    auto found = value.find(character, static_cast<size_t>(start));
    return found == std::string::npos ? -1 : static_cast<int>(found);
  }
  String substring(int start, int end) const { return value.substr(start, end - start); }
  void trim() {
    auto start = value.find_first_not_of(" \r\n\t");
    auto end = value.find_last_not_of(" \r\n\t");
    value = start == std::string::npos ? "" : value.substr(start, end - start + 1);
  }
  void toCharArray(char *out, size_t capacity) const {
    size_t count = std::min(value.size(), capacity - 1);
    memcpy(out, value.data(), count); out[count] = '\0';
  }
  String &operator+=(const String &other) { value += other.value; return *this; }
  String &operator+=(char other) { value += other; return *this; }
  friend String operator+(const String &a, const String &b) { return a.value + b.value; }
  friend bool operator==(const String &a, const String &b) { return a.value == b.value; }
  friend bool operator!=(const String &a, const String &b) { return !(a == b); }
};
#define F(text) text
enum { HTTP_GET, HTTP_POST, WIFI_AP };
using Fields = std::map<std::string, String>;
struct FakeServer {
  std::map<std::pair<std::string, int>, std::function<void()>> routes;
  Fields args, requestHeaders, responseHeaders;
  int status = 0;
  String body;
  void on(const char *path, int method, std::function<void()> handler) { routes[{path, method}] = handler; }
  void collectHeaders(const char *) {}
  void begin() {}
  bool hasArg(const String &key) { return args.count(key.value) != 0; }
  String arg(const String &key) { return args[key.value]; }
  String header(const String &key) { return requestHeaders[key.value]; }
  void sendHeader(const String &key, const String &value) { responseHeaders[key.value] = value; }
  void send(int code, const char *, const String &text) { status = code; body = text; }
  void send_P(int code, const char *, const char *text, size_t length) { status = code; body = std::string(text, length); }
  void request(const char *path, int method, Fields fields = {}, String cookie = "") {
    args = fields; requestHeaders = {{"Cookie", cookie}}; responseHeaders.clear(); status = 0; body = "";
    routes.at({path, method})();
    assert(status != 0);
  }
} server;
struct FakeWifi {
  String password;
  void mode(int) {}
  void softAP(const char *, const char *pw) { password = pw; }
} WiFi;
uint32_t nowMs = 0;
uint32_t millis() { return nowMs; }
Config config = {};
KeyboardConfig keyboardConfig = defaultKeyboardConfig();
bool stableState[IN_COUNT] = {};
bool commitSuccess = true;
int commits = 0;
bool saveConfig() { ++commits; return commitSuccess; }
#include "../btpad_pico/WebPortal.h"

String cookie() { return "BTPadSession=" + sessionToken; }
bool contains(const String &text, const char *part) { return text.value.find(part) != std::string::npos; }
void signIn(const char *password) {
  nowMs += 1000;
  server.request("/login", HTTP_POST, {{"csrf", loginCsrf}, {"pw", password}});
  assert(server.status == 303);
}
Fields settings(int slot) {
  Fields fields = {{"csrf", sessionCsrf}, {"profile", String(slot)}, {"mode", "4"},
    {"name", "Edited"}, {"turbo", "12"}, {"socd", "0"}};
  for (const PinDef &pin : PIN_MAP) {
    if (pin.id != IN_SPECIAL) fields["map" + std::to_string(pin.id)] = String(pin.id);
  }
  return fields;
}
int main() {
  config.magic = CONFIG_MAGIC;
  memcpy(config.adminPassword, "changeme", 9);
  for (Profile &profile : config.profiles) {
    memcpy(profile.name, "Default", 8);
    profile.turboHz = 12;
    for (uint8_t i = 0; i < IN_COUNT; ++i) profile.remap[i] = i;
  }
  startWebConfig();
  server.request("/inputs", HTTP_GET);
  assert(server.status == 401);
  assert(WiFi.password == "changeme");
  server.request("/", HTTP_GET);
  assert(!contains(server.body, "changeme"));
  assert(!contains(server.body, "Default"));
  server.request("/save", HTTP_POST, settings(0));
  assert(server.status == 401 && commits == 0);
  server.request("/login", HTTP_POST, {{"pw", "changeme"}});
  assert(server.status == 403);
  server.request("/login", HTTP_POST, {{"csrf", loginCsrf}, {"pw", "incorrect"}});
  assert(server.status == 401);
  server.request("/login", HTTP_POST, {{"csrf", loginCsrf}, {"pw", "changeme"}});
  assert(server.status == 429);
  signIn("changeme");
  assert(contains(server.responseHeaders["Set-Cookie"], "HttpOnly; SameSite=Strict"));
  server.request("/inputs", HTTP_GET, {}, cookie());
  assert(server.status == 403);
  server.request("/", HTTP_GET, {}, cookie());
  assert(contains(server.body, "Replace the initial password"));
  assert(!contains(server.body, "Default"));
  server.request("/save", HTTP_POST, settings(0), cookie());
  assert(server.status == 403 && commits == 0);
  server.request("/password", HTTP_POST, {{"pw", "test-admin-482!"}, {"confirm", "test-admin-482!"}}, cookie());
  assert(server.status == 403 && commits == 0);
  server.request("/password", HTTP_POST, {{"csrf", sessionCsrf}, {"pw", "123456789012345678901234"}, {"confirm", "123456789012345678901234"}}, cookie());
  assert(server.status == 400);
  server.request("/password", HTTP_POST, {{"csrf", sessionCsrf}, {"pw", "changeme"}, {"confirm", "changeme"}}, cookie());
  assert(server.status == 400);
  commitSuccess = false;
  server.request("/password", HTTP_POST, {{"csrf", sessionCsrf}, {"pw", "test-admin-482!"}, {"confirm", "test-admin-482!"}}, cookie());
  assert(server.status == 500 && needsPasswordChange());
  commitSuccess = true;
  String oldCookie = cookie();
  server.request("/password", HTTP_POST, {{"csrf", sessionCsrf}, {"pw", "test-admin-482!"}, {"confirm", "test-admin-482!"}}, oldCookie);
  assert(server.status == 303 && !needsPasswordChange());
  server.request("/save", HTTP_POST, settings(0), oldCookie);
  assert(server.status == 401);
  signIn("test-admin-482!");
  server.request("/", HTTP_GET, {{"edit", "2"}}, cookie());
  assert(contains(server.body, "name=profile value='2'"));
  assert(!contains(server.body, "test-admin-482!"));
  assert(server.responseHeaders["Cache-Control"] == "no-store");
  assert(contains(server.responseHeaders["Content-Security-Policy"], "script-src 'self'; connect-src 'self'"));
  assert(contains(server.body, "Profile mapper"));
  assert(!contains(server.body, "class=wiring"));
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    const std::string badge = "data-input='" + std::to_string(i) + "'";
    const auto first = server.body.value.find(badge);
    assert(first != std::string::npos && server.body.value.find(badge, first + 1) == std::string::npos);
  }
  assert(!contains(server.body, "name=map16")); // Special remains read-only.
  assert(contains(server.body, "href=/favicon.png"));
  assert(contains(server.responseHeaders["Content-Security-Policy"], "img-src 'self'"));
  FILE *preview = nullptr;
  if (fopen_s(&preview, "portal-preview.html", "wb") == 0) {
    fwrite(server.body.value.data(), 1, server.body.value.size(), preview);
    fclose(preview);
  }
  assert(contains(server.body, "GP14 SPECIAL"));
  assert(contains(server.body, "GP21 CAPTURE"));
  const int beforeTestCommits = commits;
  const Config beforeTestConfig = config;
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    stableState[i] = true;
    server.request("/inputs", HTTP_GET, {}, cookie());
    assert(server.status == 200);
    assert(server.body == "{\"pressed\":" + String(1 << i) + "}");
    stableState[i] = false;
  }
  stableState[IN_UP] = stableState[IN_DOWN] = true;
  server.request("/inputs", HTTP_GET, {}, cookie());
  assert(server.body == "{\"pressed\":3}"); // Opposites remain visible before SOCD.
  stableState[IN_UP] = stableState[IN_DOWN] = false;
  server.request("/inputs", HTTP_GET, {}, cookie());
  assert(server.body == "{\"pressed\":0}");
  assert(commits == beforeTestCommits && memcmp(&config, &beforeTestConfig, sizeof(config)) == 0);
  server.request("/wiring-test.js", HTTP_GET);
  assert(server.status == 200 && contains(server.body, "fetch('/inputs'"));
  server.request("/favicon.png", HTTP_GET);
  assert(server.status == 200 && server.body.length() == sizeof(PORTAL_ICON));
  assert(memcmp(server.body.value.data(), PORTAL_ICON, sizeof(PORTAL_ICON)) == 0);
  server.request("/background.jpg", HTTP_GET);
  assert(server.status == 200 && server.body.length() == sizeof(PORTAL_BACKGROUND));
  Fields keyFields = {{"csrf", sessionCsrf}, {"profile", "2"}};
  for (uint8_t i=0;i<IN_COUNT;++i) if (i!=IN_SPECIAL) keyFields["key"+std::to_string(i)]=String(keyboardConfig.maps[2][i]);
  keyFields["key4"]="225"; // Left Shift.
  const Config gamepadBeforeKeys = config;
  const KeyboardConfig keysBefore = keyboardConfig;
  server.request("/keyboard", HTTP_POST, keyFields);
  assert(server.status==401);
  commitSuccess=false;
  server.request("/keyboard", HTTP_POST, keyFields, cookie());
  assert(server.status==500 && memcmp(&keyboardConfig,&keysBefore,sizeof(keysBefore))==0);
  commitSuccess=true;
  keyFields["key4"]="3";
  server.request("/keyboard", HTTP_POST, keyFields, cookie()); assert(server.status==400);
  keyFields["key4"]="225";
  server.request("/keyboard", HTTP_POST, keyFields, cookie());
  assert(server.status==303 && validKeyboardConfig(keyboardConfig));
  assert(keyboardConfig.maps[2][IN_SOUTH]==225 && keyboardConfig.maps[0][IN_SOUTH]==29);
  assert(memcmp(&config,&gamepadBeforeKeys,sizeof(config))==0);
  server.request("/", HTTP_GET, {{"edit", "invalid"}}, cookie());
  assert(server.status == 400);
  Fields fields = settings(2);
  fields["mode"] = "8";
  server.request("/save", HTTP_POST, fields, cookie());
  assert(server.status == 303 && config.defaultMode == MODE_XBOX_CLASSIC);
  server.request("/", HTTP_GET, {}, cookie());
  assert(contains(server.body, "Xbox Classic BT (experimental)"));
  for (int unavailable : {1, 2}) {
    fields["mode"] = String(unavailable);
    server.request("/save", HTTP_POST, fields, cookie());
    assert(server.status == 400);
  }
  fields = settings(2);
  fields["name"] = "<b>&\"'";
  server.request("/save", HTTP_POST, fields, cookie());
  assert(server.status == 303 && config.activeProfile == 0);
  assert(strcmp(config.profiles[0].name, "Default") == 0);
  server.request("/", HTTP_GET, {{"edit", "2"}}, cookie());
  assert(contains(server.body, "&lt;b&gt;&amp;&quot;&#39;"));
  Config before = config;
  fields = settings(2);
  fields["turbo"] = "12garbage";
  server.request("/save", HTTP_POST, fields, cookie());
  assert(server.status == 400 && memcmp(&before, &config, sizeof(config)) == 0);
  fields = settings(2); fields.erase("map4");
  server.request("/save", HTTP_POST, fields, cookie());
  assert(server.status == 400);
  fields = settings(2); fields["map4"] = String(IN_SPECIAL);
  server.request("/save", HTTP_POST, fields, cookie());
  assert(server.status == 400);
  fields = settings(2); fields["csrf"] = "wrong";
  server.request("/save", HTTP_POST, fields, cookie());
  assert(server.status == 403);
  server.request("/save", HTTP_POST, settings(2), "FakeBTPadSession=" + sessionToken);
  assert(server.status == 401);
  commitSuccess = false;
  server.request("/save", HTTP_POST, settings(2), cookie());
  assert(server.status == 500 && memcmp(&before, &config, sizeof(config)) == 0);
  commitSuccess = true;
  fields = settings(2); fields["activate"] = "on";
  server.request("/save", HTTP_POST, fields, "other=1; " + cookie() + "; another=2");
  assert(server.status == 303 && config.activeProfile == 2);
  oldCookie = cookie();
  server.request("/logout", HTTP_POST, {{"csrf", sessionCsrf}}, oldCookie);
  assert(server.status == 303);
  server.request("/save", HTTP_POST, settings(2), oldCookie);
  assert(server.status == 401);
  nowMs = UINT32_MAX - 10000;
  signIn("test-admin-482!");
  oldCookie = cookie();
  nowMs += SESSION_MS - 1;
  server.request("/", HTTP_GET, {}, oldCookie);
  assert(contains(server.body, "Edit profile"));
  nowMs += 1;
  server.request("/inputs", HTTP_GET, {}, oldCookie);
  assert(server.status == 401);
  server.request("/save", HTTP_POST, settings(2), oldCookie);
  assert(server.status == 401);
  signIn("test-admin-482!");
  // Profile lifecycle keeps at least one and rolls back both maps on failure.
  Fields create{{"csrf", sessionCsrf}, {"name", "New game"}};
  server.request("/profile/new", HTTP_POST, create, cookie());
  assert(server.status == 409);
  Fields remove{{"csrf", sessionCsrf}, {"profile", "2"}, {"confirm", "on"}};
  Config beforeDelete = config; KeyboardConfig keysBeforeDelete = keyboardConfig;
  server.request("/profile/delete", HTTP_POST, remove); assert(server.status == 401);
  auto noCsrf = remove; noCsrf.erase("csrf");
  server.request("/profile/delete", HTTP_POST, noCsrf, cookie()); assert(server.status == 403);
  commitSuccess = false;
  server.request("/profile/delete", HTTP_POST, remove, cookie());
  assert(server.status == 500 && memcmp(&config, &beforeDelete, sizeof(config)) == 0 && memcmp(&keyboardConfig, &keysBeforeDelete, sizeof(keyboardConfig)) == 0);
  commitSuccess = true;

  server.request("/profile/delete", HTTP_POST, remove, cookie());
  assert(server.status == 303 && !profileExists(config, 2));
  server.request("/profile/new", HTTP_POST, create, cookie());
  assert(server.status == 303 && strcmp(config.profiles[2].name, "New game") == 0);
  assert(keyboardConfig.maps[2][IN_SOUTH] == defaultKeyboardConfig().maps[2][IN_SOUTH]);
  remove["profile"] = "2"; server.request("/profile/delete", HTTP_POST, remove, cookie());
  remove["profile"] = "1"; server.request("/profile/delete", HTTP_POST, remove, cookie());
  remove["profile"] = "0"; server.request("/profile/delete", HTTP_POST, remove, cookie());
  assert(server.status == 409 && isConfigValid(config));
  puts("Portal route tests passed (mock HTTP, clock, entropy and storage).");
}
