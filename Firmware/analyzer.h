#pragma once
#include <Arduino.h>
#include "config.h"

// Lapisan analisis audio yang dipakai SEMUA visualizer.
// Input : vReal[] berisi magnitudo FFT (diisi audio.cpp).
// Output: nilai ternormalisasi 0..1 yang sudah di-smooth (attack cepat, release pelan).

extern float spectrum[NUM_BANDS];   // band logaritmik, ~86 Hz .. ~10 kHz (index 0 = bass)
extern float bassEnergy;            // 0..1 energi bass (smooth)
extern float trebleEnergy;          // 0..1 energi treble (smooth)
extern float overallLevel;          // 0..1 rata-rata semua band (smooth)
extern float beatPulse;             // 1.0 saat kick terdeteksi, meluruh ~140 ms
extern bool  beatNow;               // true hanya pada frame saat beat terdeteksi

// fresh   : ada jendela FFT baru di vReal pada frame ini
// flowing : sumber audio masih aktif (kalau false, spectrum meluruh ke nol)
void analyzerUpdate(bool fresh, bool flowing);

// true kalau sudah > IDLE_AFTER_MS tanpa suara berarti
bool analyzerIdle();
