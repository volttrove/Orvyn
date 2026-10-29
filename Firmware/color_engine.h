#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

// CT_ORIGINAL -> warna bawaan mode
// CT_SOLID    -> RGB tetap
// CT_RAINBOW  -> gradasi pelangi
// CT_PEAKHUE  -> hue mengikuti level audio
// CT_FLOW     -> hue mengalir cepat
// CT_PULSE    -> hue berayun + kecerahan berdenyut
// CT_CYCLE    -> seluruh visual berganti hue ~10 detik

enum ColorType : uint8_t
{
    CT_ORIGINAL = 0,
    CT_SOLID,
    CT_RAINBOW,
    CT_PEAKHUE,
    CT_FLOW,
    CT_PULSE,
    CT_CYCLE
};

struct ModeColor
{
    uint8_t type = CT_ORIGINAL;
    CRGB solid = CRGB(255, 255, 255);
};

extern ModeColor modeColor[MODE_TOTAL];

void updateColorEngine();
CRGB dynColor(uint8_t type, CRGB solid, uint8_t pos, uint8_t bri);
CRGB colorFor(uint8_t m, uint8_t pos, uint8_t bri, CRGB orig);
CRGB tint(uint8_t m, CRGB orig, uint8_t pos);
void recolorLit(uint8_t m);
int parseMode(const String &name);
