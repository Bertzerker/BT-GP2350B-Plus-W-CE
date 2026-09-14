#pragma once
#include <pico/rand.h>
#include "WiringTest.h"

// One administrator session; tokens are regenerated at login and password change.
String loginCsrf, sessionToken, sessionCsrf;
uint32_t sessionStarted = 0, lastLoginAttempt = 0;
bool loginAttempted = false;
constexpr uint32_t SESSION_MS = 15 * 60 * 1000;

String randomToken() {
  char token[33];
  for (int i = 0; i < 4; ++i) {
    snprintf(token + i * 8, 9, "%08lx", static_cast<unsigned long>(get_rand_32()));
  }
  return String(token);
}

String escapeHtml(const String &value) {
  String result;
  for (unsigned int i = 0; i < value.length(); ++i) {
    switch (value[i]) {
      case '&': result += "&amp;"; break;
      case '<': result += "&lt;"; break;
      case '>': result += "&gt;"; break;
      case '\"': result += "&quot;"; break;
      case '\'': result += "&#39;"; break;
      default: result += value[i];
    }
  }
  return result;
}

bool parseNumber(const String &text, uint8_t maximum, uint8_t &result) {
  if (text.length() == 0 || text.length() > 3) return false;
  unsigned int value = 0;
  for (unsigned int i = 0; i < text.length(); ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    value = value * 10 + text[i] - '0';
  }
  if (value > maximum) return false;
  result = static_cast<uint8_t>(value);
  return true;
}

bool needsPasswordChange() { return strcmp(config.adminPassword, "changeme") == 0; }

void portalHeaders() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("Content-Security-Policy", "default-src 'none'; script-src 'self'; connect-src 'self'; style-src 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'; base-uri 'none'");
}

void portalRedirect() {
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

bool authorized() {
  if (sessionToken.length() == 0) return false;
  if (static_cast<uint32_t>(millis() - sessionStarted) >= SESSION_MS) {
    sessionToken = "";
    sessionCsrf = "";
    return false;
  }
  String cookies = server.header("Cookie");
  int start = 0;
  while (start < static_cast<int>(cookies.length())) {
    int end = cookies.indexOf(';', start);
    if (end < 0) end = cookies.length();
    String cookie = cookies.substring(start, end);
    cookie.trim();
    if (cookie == "BTPadSession=" + sessionToken) return true;
    start = end + 1;
  }
  return false;
}

bool requireWrite(bool allowInitialSetup = false) {
  portalHeaders();
  if (!authorized()) { server.send(401, "text/plain", "Sign in first."); return false; }
  if (!server.hasArg("csrf") || server.arg("csrf") != sessionCsrf) {
    server.send(403, "text/plain", "Form expired. Reload the page."); return false;
  }
  if (!allowInitialSetup && needsPasswordChange()) {
    server.send(403, "text/plain", "Change the initial password first."); return false;
  }
  return true;
}

String pageStart() {
  return F("<!doctype html><html lang=en><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
           "<title>BTPad setup</title><style>body{font-family:system-ui;margin:24px;max-width:760px}"
           "label{display:block;margin:12px 0}input,select,button{font:inherit;padding:8px}"
           "td,th{padding:6px;text-align:left}.pins{display:flex;flex-wrap:wrap;gap:8px}"
           ".pin{display:inline-block;padding:8px;border:2px solid #777;border-radius:8px;background:#f3f3f3;color:#222}"
           ".pin.pressed{background:#176b38;color:white;border-color:#124f2a}"
           ".pin-state{display:block;font-size:.75em}.wiring{padding:16px;border:1px solid #aaa;border-radius:12px}"
           "</style><h1>BTPad setup</h1>");
}

String pinBadge(const PinDef &pin) {
  return "<span class=pin data-input='" + String(pin.id) + "'>GP" + String(pin.gpio) + " " + pin.name +
      "<span class=pin-state>Released</span></span>";
}

String csrfField(const String &token) {
  return "<input type=hidden name=csrf value='" + token + "'>";
}

String passwordForm() {
  return "<h2>Change password</h2><form method=post action=/password>" + csrfField(sessionCsrf) +
      "<label>New password (8–23 characters)<input type=password name=pw minlength=8 maxlength=23 required autocomplete=new-password></label>"
      "<label>Confirm password<input type=password name=confirm required autocomplete=new-password></label>"
      "<button>Change password</button></form><p>The new password also becomes the Wi-Fi password at the next restart.</p>";
}

String htmlPage(uint8_t editProfile) {
  const Profile &profile = config.profiles[editProfile];
  String page = pageStart();
  page += "<section class=wiring><h2>Live wiring test</h2>"
      "<p>Hold a button or move the joystick to light up its GPIO. These are physical inputs, before remapping, SOCD or turbo. "
      "Special and Sync can be tested here without triggering their shortcuts. No settings are saved by this test.</p>"
      "<p id=wiring-status role=status>Connecting to the controller…</p><noscript>Enable JavaScript to see live inputs.</noscript><div class=pins>";
  for (const PinDef &pin : PIN_MAP) page += pinBadge(pin);
  page += "</div></section><script src=/wiring-test.js defer></script>";
  page += "<p>Generic HID and DirectInput use standard Bluetooth HID. Native XInput, Switch and PS3 are not implemented. Guide is an ordinary gamepad button.</p>";
  page += "<form method=get action=/><label>Edit profile<select name=edit>";
  for (uint8_t i = 0; i < 3; ++i) {
    page += "<option value='" + String(i) + "'" + (i == editProfile ? " selected" : "") + ">" + escapeHtml(config.profiles[i].name) + "</option>";
  }
  page += "</select></label><button>Load profile</button></form>";
  page += "<form method=post action=/save>" + csrfField(sessionCsrf);
  page += "<input type=hidden name=profile value='" + String(editProfile) + "'>";
  page += "<label>Default mode<select name=mode>";
  for (uint8_t i = 0; i < MODE_WEB; ++i) {
    if (!isSelectableMode(static_cast<HidMode>(i))) continue;
    page += "<option value='" + String(i) + "'" + (i == config.defaultMode ? " selected" : "") + ">" + MODE_NAMES[i] + "</option>";
  }
  page += "</select></label><label>Profile name<input name=name maxlength=19 required value='" + escapeHtml(profile.name) + "'></label>";
  page += "<label><input type=checkbox name=activate" + String(editProfile == config.activeProfile ? " checked" : "") + ">Use this profile</label>";
  page += "<label>Turbo Hz<input name=turbo type=number min=2 max=30 required value='" + String(profile.turboHz) + "'></label>";
  page += "<label>SOCD<select name=socd>";
  const char *names[] = {"Neutral", "Up priority", "Last input"};
  for (uint8_t i = 0; i < 3; ++i) {
    page += "<option value='" + String(i) + "'" + (i == profile.socd ? " selected" : "") + ">" + names[i] + "</option>";
  }
  page += "</select></label><table><tr><th>Physical input</th><th>Maps to</th><th>Turbo</th></tr>";
  for (const PinDef &pin : PIN_MAP) {
    if (pin.id == IN_SPECIAL) continue;
    page += "<tr><td>" + pinBadge(pin) + "</td><td><select name=map" + String(pin.id) + ">";
    for (uint8_t target = 0; target < IN_COUNT; ++target) {
      if (target == IN_SPECIAL) continue;
      page += "<option value='" + String(target) + "'" + (profile.remap[pin.id] == target ? " selected" : "") + ">" + MODE_LABELS[config.defaultMode].labels[target] + "</option>";
    }
    page += "</select></td><td>";
    if (pin.id >= IN_A && pin.id <= IN_RS) {
      page += "<input type=checkbox name=turbo" + String(pin.id) + ((profile.turboMask & (1u << pin.id)) ? " checked" : "") + ">";
    }
    page += "</td></tr>";
  }
  page += "</table><button>Save settings</button></form><p>Restart to use the selected default mode.</p>";
  page += passwordForm();
  page += "<form method=post action=/logout>" + csrfField(sessionCsrf) + "<button>Sign out</button></form></html>";
  return page;
}

void startWebConfig() {
  loginCsrf = randomToken();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BTPad-Setup", config.adminPassword);
  server.collectHeaders("Cookie");
  server.on("/wiring-test.js", HTTP_GET, []() {
    portalHeaders();
    server.send(200, "application/javascript", WIRING_TEST_JS);
  });
  server.on("/inputs", HTTP_GET, []() {
    portalHeaders();
    if (!authorized()) { server.send(401, "text/plain", "Sign in first."); return; }
    if (needsPasswordChange()) { server.send(403, "text/plain", "Change the initial password first."); return; }
    uint32_t mask = 0;
    for (uint8_t i = 0; i < IN_COUNT; ++i) {
      if (stableState[i]) mask |= 1u << i;
    }
    server.send(200, "application/json", "{\"pressed\":" + String(static_cast<int>(mask)) + "}");
  });
  server.on("/", HTTP_GET, []() {
    portalHeaders();
    if (!authorized()) {
      server.send(200, "text/html", pageStart() + "<form method=post action=/login>" + csrfField(loginCsrf) +
          "<label>Password<input type=password name=pw required autocomplete=current-password></label><button>Sign in</button></form></html>");
      return;
    }
    if (needsPasswordChange()) {
      server.send(200, "text/html", pageStart() + "<p>Replace the initial password before configuring the controller.</p>" + passwordForm() + "</html>");
      return;
    }
    uint8_t edit = config.activeProfile;
    if (server.hasArg("edit") && !parseNumber(server.arg("edit"), 2, edit)) {
      server.send(400, "text/plain", "Invalid profile."); return;
    }
    server.send(200, "text/html", htmlPage(edit));
  });
  server.on("/login", HTTP_POST, []() {
    portalHeaders();
    if (!server.hasArg("csrf") || server.arg("csrf") != loginCsrf) {
      server.send(403, "text/plain", "Reload the sign-in page."); return;
    }
    if (loginAttempted && static_cast<uint32_t>(millis() - lastLoginAttempt) < 1000) {
      server.send(429, "text/plain", "Wait a second before trying again."); return;
    }
    loginAttempted = true;
    lastLoginAttempt = millis();
    if (!server.hasArg("pw") || server.arg("pw") != config.adminPassword) {
      server.send(401, "text/plain", "Incorrect password."); return;
    }
    sessionToken = randomToken();
    sessionCsrf = randomToken();
    sessionStarted = millis();
    loginCsrf = randomToken();
    server.sendHeader("Set-Cookie", "BTPadSession=" + sessionToken + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=900");
    portalRedirect();
  });
  server.on("/password", HTTP_POST, []() {
    if (!requireWrite(true)) return;
    String password = server.arg("pw");
    bool printable = true;
    for (unsigned int i = 0; i < password.length(); ++i) {
      if (password[i] < 32 || password[i] > 126) printable = false;
    }
    if (!printable || password.length() < 8 || password.length() > 23 || password == "changeme" || password != server.arg("confirm")) {
      server.send(400, "text/plain", "Use 8–23 printable ASCII characters, replace the initial password, and match the confirmation."); return;
    }
    Config previous = config;
    password.toCharArray(config.adminPassword, sizeof(config.adminPassword));
    if (!saveConfig()) {
      config = previous;
      server.send(500, "text/plain", "Storage write failed. Password was not changed."); return;
    }
    sessionToken = "";
    sessionCsrf = "";
    loginCsrf = randomToken();
    server.sendHeader("Set-Cookie", "BTPadSession=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    portalRedirect();
  });
  server.on("/save", HTTP_POST, []() {
    if (!requireWrite()) return;
    Config candidate = config;
    uint8_t mode = 0, profileId = 0, turbo = 0, socd = 0;
    if (!parseNumber(server.arg("mode"), MODE_KEYBOARD, mode) ||
        !parseNumber(server.arg("profile"), 2, profileId) ||
        !parseNumber(server.arg("turbo"), 30, turbo) || turbo < 2 ||
        !parseNumber(server.arg("socd"), SOCD_LAST_INPUT, socd) ||
        server.arg("name").length() == 0 || server.arg("name").length() > 19) {
      server.send(400, "text/plain", "Invalid settings; nothing saved."); return;
    }
    candidate.defaultMode = static_cast<HidMode>(mode);
    if (!isSelectableMode(candidate.defaultMode)) {
      server.send(400, "text/plain", "That output mode is not implemented."); return;
    }
    if (server.hasArg("activate")) candidate.activeProfile = profileId;
    Profile &profile = candidate.profiles[profileId];
    server.arg("name").toCharArray(profile.name, sizeof(profile.name));
    profile.turboHz = turbo;
    profile.socd = static_cast<SocdMode>(socd);
    profile.turboMask = 0;
    for (const PinDef &pin : PIN_MAP) {
      if (pin.id == IN_SPECIAL) continue;
      if (!parseNumber(server.arg("map" + String(pin.id)), IN_COUNT - 1, profile.remap[pin.id])) {
        server.send(400, "text/plain", "Invalid mapping; nothing saved."); return;
      }
      if (pin.id >= IN_A && pin.id <= IN_RS && server.hasArg("turbo" + String(pin.id))) profile.turboMask |= 1u << pin.id;
    }
    if (!isConfigValid(candidate)) { server.send(400, "text/plain", "Invalid configuration; nothing saved."); return; }
    Config previous = config;
    config = candidate;
    if (!saveConfig()) {
      config = previous;
      server.send(500, "text/plain", "Storage write failed; previous settings retained."); return;
    }
    server.sendHeader("Location", "/?edit=" + String(profileId));
    server.send(303, "text/plain", "");
  });
  server.on("/logout", HTTP_POST, []() {
    if (!requireWrite(true)) return;
    sessionToken = "";
    sessionCsrf = "";
    server.sendHeader("Set-Cookie", "BTPadSession=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    portalRedirect();
  });
  server.begin();
}
