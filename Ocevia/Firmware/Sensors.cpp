#include "Sensors.h"
#include "Config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>

// ---------------- DS18B20 ----------------
static OneWire oneWire(PIN_DS18B20);
static DallasTemperature ds(&oneWire);
static DeviceAddress dsAddr;
static bool haveAddr = false;
static bool converting = false;
static bool everValid = false;
static float lastTemp = 0.0f;
static uint32_t lastRequest = 0;
static uint32_t lastScan = 0;
static uint32_t lastValidMs = 0;

static void scanSensor() {
  ds.begin();
  haveAddr = ds.getAddress(dsAddr, 0);
  ds.setWaitForConversion(false);  // jangan block loop selama ~750 ms
  lastScan = millis();
}

static bool plausible(float v) {
  return v != DEVICE_DISCONNECTED_C && v > -55.0f && v < 125.0f && v != TEMP_POWER_ON_VALUE;
}

static void updateTemperature() {
  uint32_t t = millis();

  if (!haveAddr) {
    if (t - lastScan >= SENSOR_RESCAN_MS) scanSensor();
    return;
  }

  if (!converting) {
    if (t - lastRequest >= TEMP_INTERVAL_MS) {
      ds.requestTemperatures();
      lastRequest = t;
      converting = true;
    }
  } else if (t - lastRequest >= TEMP_CONVERSION_MS) {
    converting = false;
    float v = ds.getTempC(dsAddr);
    if (plausible(v)) {
      lastTemp = v;
      lastValidMs = t;
      everValid = true;
    }
  }
}

bool tempValid() {
  return everValid && (millis() - lastValidMs) <= TEMP_STALE_MS;
}

bool tempFault() {
  return !tempValid() && millis() > SENSOR_BOOT_GRACE_MS;
}

float tempValue() { return lastTemp; }

// ---------------- DS1307 ----------------
static RTC_DS1307 rtc;
static bool rtcAvail = false;
static bool rtcRunning = false;
static ClockTime cached = {0, 0, 0, 0, 0, 0};
static uint32_t lastRtcPoll = 0;
static uint32_t lastRtcTry = 0;

static void pollRtc() {
  if (!rtcAvail) {
    uint32_t t = millis();
    if (t - lastRtcTry < RTC_RETRY_MS) {
      rtcRunning = false;
      return;
    }
    lastRtcTry = t;
    rtcAvail = rtc.begin();
    if (!rtcAvail) {
      rtcRunning = false;
      return;
    }
  }

  if (rtc.isrunning()) {
    DateTime n = rtc.now();
    if (n.year() >= RTC_MIN_YEAR && n.year() <= RTC_MAX_YEAR) {
      cached.year = n.year();
      cached.month = n.month();
      cached.day = n.day();
      cached.hour = n.hour();
      cached.minute = n.minute();
      cached.second = n.second();
      rtcRunning = true;
      return;
    }
  }
  rtcRunning = false;
}

bool rtcOk() { return rtcRunning; }
ClockTime rtcTime() { return cached; }
uint16_t rtcMinuteOfDay() { return cached.hour * 60 + cached.minute; }

bool rtcSetTime(int year, int month, int day, int hour, int minute, int second) {
  if (!rtcAvail) return false;
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
  pollRtc();
  return rtcRunning;
}

// ---------------- public ----------------
void sensorsBegin() {
  scanSensor();
  lastRequest = millis() - TEMP_INTERVAL_MS;  // request pertama langsung jalan

  rtcAvail = rtc.begin();
  lastRtcTry = millis();
  Serial.println(rtcAvail ? "RTC DS1307 OK" : "RTC DS1307 TIDAK DITEMUKAN!");
  pollRtc();
  if (rtcAvail && !rtcRunning) {
    // Sengaja TIDAK diset ke waktu compile. Tunggu SETTIME dari aplikasi.
    Serial.println("RTC belum valid / kehilangan waktu. Kirim SETTIME dari aplikasi.");
  }
  lastRtcPoll = millis();
}

void sensorsUpdate() {
  updateTemperature();

  uint32_t t = millis();
  if (t - lastRtcPoll >= RTC_POLL_MS) {
    lastRtcPoll = t;
    pollRtc();
  }
}
