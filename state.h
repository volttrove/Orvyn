#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

// ---------------- Metadata lagu ----------------
// Ditulis dari task Bluetooth, dibaca dari task OLED -> dilindungi mutex.
// Memakai char[] (bukan String) supaya heap tidak terfragmentasi.
#define META_LEN 64

enum MetaField { META_TITLE = 0, META_ARTIST };

void metaInit();
void metaSet(MetaField field, const char *text);   // aman dipanggil dari task manapun
bool metaGet(char *title, size_t titleSize,
             char *artistOut, size_t artistSize,
             uint32_t *version);                    // version naik tiap ada perubahan

// ---------------- Teks berjalan ----------------
extern String runningText;

// ---------------- Audio / FFT ----------------
extern float vReal[SAMPLES];
extern float vImag[SAMPLES];
extern volatile int level;          // rata-rata |amplitudo| (info/debug)

// ---------------- Status ----------------
extern int brightness;
extern bool matrixEnabled;
extern bool isPlaying;
extern VisualMode visualizerMode;
extern bool bleConnected;
extern volatile bool a2dpConnected;
extern bool aboutActive;
extern unsigned long aboutStart;

// ---------------- LED ----------------
extern CRGB leds[NUM_LEDS];
uint16_t XY(uint8_t x, uint8_t y);
