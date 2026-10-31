#include "Relays.h"
#include "Config.h"
#include "Sensors.h"
#include "Settings.h"

struct Channel {
  uint8_t pin;
  bool state;
  bool timerActive;
  uint32_t timerEnd;
  uint32_t lastSwitch;
  bool switched;  // sudah pernah berpindah sejak boot
};

static Channel ch[CH_COUNT] = {
  {PIN_RELAY_1, false, false, 0, 0, false},
  {PIN_RELAY_2, false, false, 0, 0, false},
};

static uint32_t lastEval = 0;
static void (*changeCb)() = nullptr;

static void changed() {
  if (changeCb) changeCb();
}

static void writePin(uint8_t id) {
  bool level = RELAY_ACTIVE_LOW ? !ch[id].state : ch[id].state;
  digitalWrite(ch[id].pin, level ? HIGH : LOW);
}

static void setState(uint8_t id, bool on) {
  if (ch[id].state == on) return;
  ch[id].state = on;
  writePin(id);
  ch[id].lastSwitch = millis();
  ch[id].switched = true;
}

static void cancelTimer(uint8_t id) {
  ch[id].timerActive = false;
  ch[id].timerEnd = 0;
}

// State disimpan hanya untuk mode manual. Kalau timer aktif, simpan OFF
// supaya setelah mati lampu relay tidak ON selamanya tanpa timer.
static void persistManual(uint8_t id) {
  settingsSaveRelayState(id, ch[id].state && !ch[id].timerActive);
}

static void leaveAuto(uint8_t id) {
  if (settings.autoMode[id]) {
    settings.autoMode[id] = false;
    settingsSaveConfig();
  }
}

// ---------- logika AUTO ----------
static bool desiredFan(bool current) {
  if (tempValid()) {
    float t = tempValue();
    if (t > settings.fanOn) return true;
    if (t <= settings.fanOff) return false;
    return current;  // di dalam zona hysteresis
  }
  if (tempFault()) return FAN_FAILSAFE_ON;
  return current;
}

static bool desiredLamp(bool current) {
  if (!rtcOk()) return current;  // jam tidak valid: tahan state terakhir

  uint16_t m = rtcMinuteOfDay();
  uint16_t on = settings.lampOn;
  uint16_t off = settings.lampOff;

  if (on == off) return false;
  if (on < off) return m >= on && m < off;
  return m >= on || m < off;  // lewat tengah malam
}

void relayEvaluate(uint8_t id, bool force) {
  if (id >= CH_COUNT || !settings.autoMode[id]) return;

  bool desired = (id == CH_FAN) ? desiredFan(ch[id].state) : desiredLamp(ch[id].state);
  if (desired == ch[id].state) return;

  uint32_t t = millis();
  if (!force && ch[id].switched && (t - ch[id].lastSwitch) < RELAY_MIN_SWITCH_MS) return;

  setState(id, desired);
  Serial.printf("R%u AUTO: %s\n", (unsigned)(id + 1), desired ? "ON" : "OFF");
  changed();
}

// ---------- public ----------
void relaysPreInit() {
  for (uint8_t i = 0; i < CH_COUNT; i++) {
    // Set level OFF dulu baru jadikan output, supaya relay tidak berkedip.
    digitalWrite(ch[i].pin, RELAY_ACTIVE_LOW ? HIGH : LOW);
    pinMode(ch[i].pin, OUTPUT);
  }
}

void relaysBegin() {
  for (uint8_t i = 0; i < CH_COUNT; i++) {
    ch[i].state = settings.autoMode[i] ? false : settings.relayState[i];
    writePin(i);
  }
}

void relaysUpdate() {
  uint32_t t = millis();

  for (uint8_t i = 0; i < CH_COUNT; i++) {
    if (ch[i].timerActive && (int32_t)(t - ch[i].timerEnd) >= 0) {
      cancelTimer(i);
      setState(i, false);
      persistManual(i);
      Serial.printf("R%u TIMER FINISHED -> OFF\n", (unsigned)(i + 1));
      changed();
    }
  }

  if (t - lastEval >= RELAY_EVAL_MS) {
    lastEval = t;
    relayEvaluate(CH_FAN, false);
    relayEvaluate(CH_LAMP, false);
  }
}

bool relayState(uint8_t id) { return id < CH_COUNT && ch[id].state; }
bool relayIsAuto(uint8_t id) { return id < CH_COUNT && settings.autoMode[id]; }
bool relayTimerActive(uint8_t id) { return id < CH_COUNT && ch[id].timerActive; }

uint32_t relayRemainingSeconds(uint8_t id) {
  if (id >= CH_COUNT || !ch[id].timerActive) return 0;
  int32_t remaining = (int32_t)(ch[id].timerEnd - millis());
  return remaining <= 0 ? 0 : (uint32_t)remaining / 1000UL;
}

bool relaySetManual(uint8_t id, bool on) {
  if (id >= CH_COUNT) return false;
  bool wasAuto = settings.autoMode[id];
  leaveAuto(id);
  cancelTimer(id);
  setState(id, on);
  persistManual(id);
  changed();
  return wasAuto;
}

bool relayAddTimer(uint8_t id, uint32_t seconds) {
  if (id >= CH_COUNT) return false;
  bool wasAuto = settings.autoMode[id];
  leaveAuto(id);

  uint32_t total = relayRemainingSeconds(id) + seconds;  // perpanjang timer yang berjalan
  if (total > MAX_TIMER_SECONDS) total = MAX_TIMER_SECONDS;

  ch[id].timerEnd = millis() + total * 1000UL;
  ch[id].timerActive = true;
  setState(id, true);
  persistManual(id);
  changed();
  return wasAuto;
}

bool relayEnableAuto(uint8_t id) {
  if (id >= CH_COUNT) return false;
  if (id == CH_LAMP && !settings.lampSet) return false;

  cancelTimer(id);
  settings.autoMode[id] = true;
  settingsSaveConfig();
  relayEvaluate(id, true);
  changed();
  return true;
}

void relaysSetChangeCallback(void (*cb)()) { changeCb = cb; }
