#ifndef FONTJP16X16_H
#define FONTJP16X16_H

#include <Arduino.h>
#include <FastLED.h>

#ifndef MATRIX_WIDTH
#define MATRIX_WIDTH 16
#endif

#ifndef MATRIX_HEIGHT
#define MATRIX_HEIGHT 16
#endif

struct Glyph16
{
    uint16_t row[16];
};

extern const Glyph16 FONT_YIN PROGMEM;
extern const Glyph16 FONT_GAKU PROGMEM;
extern const Glyph16 FONT_DEN PROGMEM;
extern const Glyph16 FONT_CHI PROGMEM;

void drawGlyph16(
    const Glyph16 &glyph,
    int x,
    int y,
    CRGB color);

#endif // FONTJP16X16_H
