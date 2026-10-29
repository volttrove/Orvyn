#pragma once

#include <FastLED.h>

extern const uint8_t font5x7[][5];

void drawChar(
    char c,
    int x,
    int y,
    CRGB color
);

void drawString(
    const char *text,
    int x,
    int y,
    CRGB color
);
