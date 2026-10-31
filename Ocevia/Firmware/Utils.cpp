#include "Utils.h"
#include "Config.h"
#include <math.h>
#include <ctype.h>

void formatClock(uint16_t minuteOfDay, char *out, size_t size) {
  snprintf(out, size, "%02u:%02u", (unsigned)(minuteOfDay / 60), (unsigned)(minuteOfDay % 60));
}

static bool allDigits(const String &s) {
  if (s.length() == 0) return false;
  for (size_t i = 0; i < s.length(); i++) {
    if (!isdigit((unsigned char)s[i])) return false;
  }
  return true;
}

bool parseClock(String input, uint16_t &minuteOfDay) {
  input.trim();
  int comma = input.indexOf(',');
  if (comma < 1 || comma >= (int)input.length() - 1) return false;

  String hourText = input.substring(0, comma);
  String minuteText = input.substring(comma + 1);

  if (hourText.length() > 2 || minuteText.length() != 2) return false;
  if (!allDigits(hourText) || !allDigits(minuteText)) return false;

  int hour = hourText.toInt();
  int minute = minuteText.toInt();
  if (hour > 23 || minute > 59) return false;

  minuteOfDay = hour * 60 + minute;
  return true;
}

bool parseTemperatureValue(String input, float &value) {
  input.trim();
  if (input.length() == 0) return false;

  bool seenDigit = false;
  bool seenDot = false;
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c >= '0' && c <= '9') seenDigit = true;
    else if (c == '.' && !seenDot) seenDot = true;
    else return false;
  }
  if (!seenDigit) return false;

  float parsed = input.toFloat();
  if (!isfinite(parsed)) return false;
  value = parsed;
  return true;
}

bool parseTimerSeconds(String input, uint32_t &seconds) {
  input.trim();
  if (input.length() == 0 || input.length() > 6 || !allDigits(input)) return false;

  long v = input.toInt();
  if (v < 1 || v > (long)MAX_TIMER_SECONDS) return false;
  seconds = (uint32_t)v;
  return true;
}

bool isValidDate(int year, int month, int day) {
  if (year < 2000 || month < 1 || month > 12 || day < 1) return false;
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int maxDay = days[month - 1];
  bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  if (month == 2 && leap) maxDay = 29;
  return day <= maxDay;
}
