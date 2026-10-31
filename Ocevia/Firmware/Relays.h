#pragma once
#include <Arduino.h>

enum : uint8_t { CH_FAN = 0, CH_LAMP = 1, CH_COUNT = 2 };

void relaysPreInit();   // paling awal di setup(): paksa kedua relay OFF
void relaysBegin();     // setelah settingsLoad(): pulihkan state tersimpan
void relaysUpdate();    // timer + evaluasi AUTO (rate-limited)

bool relayState(uint8_t id);
bool relayIsAuto(uint8_t id);
bool relayTimerActive(uint8_t id);
uint32_t relayRemainingSeconds(uint8_t id);

// Mengembalikan true kalau sebelumnya mode AUTO (caller bisa bunyikan buzzer).
bool relaySetManual(uint8_t id, bool on);
bool relayAddTimer(uint8_t id, uint32_t seconds);

// false kalau tidak bisa (lampu tanpa jadwal tersimpan).
bool relayEnableAuto(uint8_t id);

// Paksa evaluasi AUTO sekarang (mengabaikan interval anti-chattering).
void relayEvaluate(uint8_t id, bool force);

// Dipanggil setiap ada perubahan state/mode (dipakai untuk notify BLE).
void relaysSetChangeCallback(void (*cb)());
