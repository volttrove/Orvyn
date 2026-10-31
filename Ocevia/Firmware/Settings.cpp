#include "Settings.h"
#include "Config.h"
#include <Preferences.h>
#include <math.h>

AppSettings settings = {
  {false, false}, {false, false},
  LAMP_ON_DEFAULT, LAMP_OFF_DEFAULT, false,
  FAN_ON_DEFAULT, FAN_OFF_DEFAULT
};

static Preferences prefs;
static const char *NS = "Ocevia";

static bool fanThresholdsValid(float on, float off) {
  return isfinite(on) && isfinite(off) &&
         on >= 0.0f && on <= 60.0f &&
         off >= 0.0f && off <= 60.0f &&
         off < on;
}

void settingsLoad() {
  // Mode read-write supaya namespace otomatis dibuat saat boot pertama.
  prefs.begin(NS, false);

  settings.autoMode[0] = prefs.getBool("r1auto", false);
  settings.autoMode[1] = prefs.getBool("r2auto", false);
  settings.lampOn = prefs.getUShort("lampOn", LAMP_ON_DEFAULT);
  settings.lampOff = prefs.getUShort("lampOff", LAMP_OFF_DEFAULT);
  settings.lampSet = prefs.getBool("lampSet", false);
  settings.relayState[0] = prefs.getBool("r1state", false);
  settings.relayState[1] = prefs.getBool("r2state", false);
  settings.fanOn = prefs.getFloat("fanOn", FAN_ON_DEFAULT);
  settings.fanOff = prefs.getFloat("fanOff", FAN_OFF_DEFAULT);

  prefs.end();

  if (settings.lampOn > 1439) settings.lampOn = LAMP_ON_DEFAULT;
  if (settings.lampOff > 1439) settings.lampOff = LAMP_OFF_DEFAULT;

  if (!fanThresholdsValid(settings.fanOn, settings.fanOff)) {
    settings.fanOn = FAN_ON_DEFAULT;
    settings.fanOff = FAN_OFF_DEFAULT;
  }
}

void settingsSaveConfig() {
  prefs.begin(NS, false);
  prefs.putBool("r1auto", settings.autoMode[0]);
  prefs.putBool("r2auto", settings.autoMode[1]);
  prefs.putUShort("lampOn", settings.lampOn);
  prefs.putUShort("lampOff", settings.lampOff);
  prefs.putBool("lampSet", settings.lampSet);
  prefs.putFloat("fanOn", settings.fanOn);
  prefs.putFloat("fanOff", settings.fanOff);
  prefs.end();
}

void settingsSaveRelayState(uint8_t id, bool on) {
  if (id > 1) return;
  settings.relayState[id] = on;
  prefs.begin(NS, false);
  prefs.putBool(id == 0 ? "r1state" : "r2state", on);
  prefs.end();
}
