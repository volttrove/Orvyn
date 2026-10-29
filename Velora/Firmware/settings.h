#pragma once
#include <Arduino.h>

// Simpan brightness, mode, warna per-mode, dan running text ke NVS
// supaya tidak hilang saat reboot. Penulisan ditunda 3 detik setelah
// perubahan terakhir (mengurangi keausan flash).
void settingsLoad();
void settingsMarkDirty();
void settingsLoop();
