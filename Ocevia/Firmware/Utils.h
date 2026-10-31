#pragma once
#include <Arduino.h>

// "HH:MM" ke buffer (minimal 6 byte)
void formatClock(uint16_t minuteOfDay, char *out, size_t size);

// "18,00" -> menit sejak tengah malam
bool parseClock(String input, uint16_t &minuteOfDay);

// Desimal ketat: tolak kosong, huruf, NaN, infinity
bool parseTemperatureValue(String input, float &value);

// Hanya digit, 1..MAX_TIMER_SECONDS
bool parseTimerSeconds(String input, uint32_t &seconds);

// Validasi tanggal kalender (termasuk tahun kabisat)
bool isValidDate(int year, int month, int day);
