#pragma once
#include <Arduino.h>

struct ClockTime {
  uint16_t year;
  uint8_t month, day, hour, minute, second;
};

void sensorsBegin();
void sensorsUpdate();  // non-blocking: suhu (state machine) + RTC (di-cache)

// Suhu
bool tempValid();    // ada bacaan valid yang masih segar
bool tempFault();    // tidak valid DAN sudah lewat masa toleransi boot
float tempValue();

// RTC (hasil cache, aman dipanggil tiap loop tanpa I2C)
bool rtcOk();
ClockTime rtcTime();
uint16_t rtcMinuteOfDay();
bool rtcSetTime(int year, int month, int day, int hour, int minute, int second);
