#pragma once
#include <Arduino.h>

// Satu-satunya sumber kebenaran untuk konfigurasi yang disimpan di NVS.
struct AppSettings {
  bool autoMode[2];     // [0]=R1 kipas, [1]=R2 lampu
  bool relayState[2];   // state manual terakhir (tanpa timer)
  uint16_t lampOn;      // menit sejak tengah malam
  uint16_t lampOff;
  bool lampSet;         // jadwal pernah disimpan
  float fanOn;
  float fanOff;
};

extern AppSettings settings;

void settingsLoad();
// Simpan konfigurasi (mode AUTO, jadwal, threshold). Dipanggil jarang.
void settingsSaveConfig();
// Simpan state relay manual. Dipanggil hanya pada perubahan manual.
void settingsSaveRelayState(uint8_t id, bool on);
