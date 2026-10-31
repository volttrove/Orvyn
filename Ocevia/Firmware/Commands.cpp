#include "Commands.h"
#include "Config.h"
#include "BleLink.h"
#include "Buzzer.h"
#include "Display.h"
#include "Relays.h"
#include "Sensors.h"
#include "Settings.h"
#include "Utils.h"

static void handleFanTemp(String values) {
  int sep = values.indexOf(',');
  if (sep <= 0 || sep >= (int)values.length() - 1) {
    Serial.println("R1 TEMP ERROR: USE R1 TEMP 28.2,27.5");
    return;
  }

  float newOn = 0.0f, newOff = 0.0f;
  if (parseTemperatureValue(values.substring(0, sep), newOn) &&
      parseTemperatureValue(values.substring(sep + 1), newOff) &&
      newOn <= 60.0f && newOff <= 60.0f && newOff < newOn) {
    settings.fanOn = newOn;
    settings.fanOff = newOff;
    settingsSaveConfig();
    if (relayIsAuto(CH_FAN)) relayEvaluate(CH_FAN, true);
    Serial.printf("R1 TEMP UPDATED: ON > %.1f C, OFF <= %.1f C\n", settings.fanOn, settings.fanOff);
  } else {
    Serial.println("R1 TEMP ERROR: INVALID VALUES (0-60 C, OFF must be lower than ON)");
  }
}

// R1 ON|OFF|AUTO|TEMP a,b|<detik>   dan   R2 ON|OFF|AUTO|<detik>
static void handleRelay(uint8_t id, String arg) {
  arg.trim();
  unsigned n = id + 1;

  if (arg == "ON" || arg == "OFF") {
    if (relaySetManual(id, arg == "ON")) buzzerBeep();
    Serial.printf("R%u: MANUAL %s\n", n, arg.c_str());
  }
  else if (arg == "AUTO") {
    if (id == CH_LAMP && !settings.lampSet) {
      Serial.println("R2 AUTO ERROR: NO SAVED SCHEDULE");
      return;
    }
    bool wasAuto = relayIsAuto(id);
    if (relayEnableAuto(id)) {
      // R2 AUTO selalu beep (konfirmasi eksplisit), R1 hanya saat berpindah mode.
      if (id == CH_LAMP || !wasAuto) buzzerBeep();
      Serial.printf("R%u: AUTO ENABLED\n", n);
    }
  }
  else if (id == CH_FAN && arg.startsWith("TEMP ")) {
    handleFanTemp(arg.substring(5));
  }
  else {
    uint32_t seconds;
    if (parseTimerSeconds(arg, seconds)) {
      if (relayAddTimer(id, seconds)) buzzerBeep();
      Serial.printf("R%u TIMER: +%lu s (sisa %lu s)\n", n,
                    (unsigned long)seconds, (unsigned long)relayRemainingSeconds(id));
    } else {
      Serial.printf("R%u: INVALID TIMER (1-%lu)\n", n, (unsigned long)MAX_TIMER_SECONDS);
    }
  }
}

// R2:18,00:22,00
static void handleSchedule(String schedule) {
  int sep = schedule.indexOf(':');
  uint16_t onMinute, offMinute;

  if (sep > 0 &&
      parseClock(schedule.substring(0, sep), onMinute) &&
      parseClock(schedule.substring(sep + 1), offMinute)) {
    bool wasAuto = relayIsAuto(CH_LAMP);
    settings.lampOn = onMinute;
    settings.lampOff = offMinute;
    settings.lampSet = true;
    relayEnableAuto(CH_LAMP);  // menyimpan config + evaluasi langsung
    if (!wasAuto) buzzerBeep();

    char a[8], b[8];
    formatClock(onMinute, a, sizeof(a));
    formatClock(offMinute, b, sizeof(b));
    Serial.printf("R2 SCHEDULE: %s-%s\n", a, b);
  } else {
    Serial.println("R2: INVALID SCHEDULE (use R2:18,00:22,00)");
  }
}

// SETTIME YYYY-MM-DD HH:MM:SS
static void handleSetTime(const String &data) {
  int y, mo, d, h, mi, s;
  if (sscanf(data.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &y, &mo, &d, &h, &mi, &s) != 6 ||
      y < RTC_MIN_YEAR || y > RTC_MAX_YEAR || !isValidDate(y, mo, d) ||
      h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 || s > 59) {
    Serial.println("RTC: INVALID TIME FORMAT (YYYY-MM-DD HH:MM:SS)");
    return;
  }

  if (rtcSetTime(y, mo, d, h, mi, s)) {
    Serial.println("RTC: TIME UPDATED");
    relayEvaluate(CH_LAMP, true);
  } else {
    Serial.println("RTC: UNAVAILABLE");
  }
}

static void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  Serial.print("BLE COMMAND: ");
  Serial.println(cmd);

  if (cmd == "ABOUT") displayShowAbout();
  else if (cmd == "STATUS") { /* hanya kirim status, tanpa beep */ }
  else if (cmd.startsWith("SETTIME ")) handleSetTime(cmd.substring(8));
  else if (cmd.startsWith("R1 ")) handleRelay(CH_FAN, cmd.substring(3));
  else if (cmd.startsWith("R2:")) handleSchedule(cmd.substring(3));
  else if (cmd.startsWith("R2 ")) handleRelay(CH_LAMP, cmd.substring(3));
  else Serial.println("BLE: UNKNOWN COMMAND");

  bleRequestStatus();
}

void commandsUpdate() {
  char buf[CMD_MAX_LEN];
  for (uint8_t i = 0; i < CMD_QUEUE_LENGTH && bleDequeueCommand(buf, sizeof(buf)); i++) {
    handleCommand(String(buf));
  }
}
