#include "fontJP16x16.h"

extern uint16_t XY(uint8_t x, uint8_t y);
extern CRGB leds[];

// 音
const Glyph16 FONT_YIN PROGMEM =
{
    {
        0x0100,
        0x2FCC,
        0x7FFC,
        0x1830,
        0x0820,
        0x0C60,
        0xFFFE,
        0x0000,
        0x1FF8,
        0x3FF8,
        0x3018,
        0x3FF8,
        0x3FF8,
        0x3018,
        0x3FF8,
        0x3FF8
    }
};

// 楽
const Glyph16 FONT_GAKU PROGMEM =
{
    {
        0x0180,
        0x4182,
        0x67E6,
        0x343C,
        0x07E0,
        0x0FF8,
        0x3C3C,
        0xE7E7,
        0x87E0,
        0x0180,
        0xFFFF,
        0x07E0,
        0x0DB0,
        0x399C,
        0xF18F,
        0x4182
    }
};

// 電
const Glyph16 FONT_DEN PROGMEM =
{
    {
        0x7FFC,
        0x0100,
        0xFFFE,
        0xC182,
        0xBD7A,
        0x8102,
        0x3D7C,
        0x0100,
        0x3FFC,
        0x610C,
        0x7FFC,
        0x618C,
        0x7FFC,
        0x7FF9,
        0x2183,
        0x01FE
    }
};

// 池
const Glyph16 FONT_CHI PROGMEM =
{
    {
        0x6020,
        0x7020,
        0x1B20,
        0x0326,
        0x033E,
        0xC3F2,
        0x73E2,
        0x2F22,
        0x0322,
        0x0322,
        0x1B2E,
        0x132C,
        0x3301,
        0x6303,
        0x6303,
        0x41FE
    }
};

void drawGlyph16(
    const Glyph16 &glyph,
    int x,
    int y,
    CRGB color)
{
    for (int row = 0; row < 16; row++)
    {
        uint16_t bits = pgm_read_word(&glyph.row[row]);

        for (int col = 0; col < 16; col++)
        {
            if (bits & (1 << (15 - col)))
            {
                int xx = x + col;
                int yy = y + row;

                if (xx >= 0 && xx < MATRIX_WIDTH &&
                    yy >= 0 && yy < MATRIX_HEIGHT)
                {
                    leds[XY(xx, yy)] = color;
                }
            }
        }
    }
}
