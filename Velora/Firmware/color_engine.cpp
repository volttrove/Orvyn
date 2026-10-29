#include "color_engine.h"
#include "state.h"
#include "analyzer.h"

ModeColor modeColor[MODE_TOTAL];

static uint8_t gRainbowShift = 0;
static uint8_t gFlowHue = 0;
static uint8_t gCycleHue = 0;
static uint8_t gPulseHue = 0;
static uint8_t gPulseBri = 255;
static uint8_t gPeakHue = 160;

void updateColorEngine()
{
    uint32_t now = millis();

    gRainbowShift = now >> 6;
    gFlowHue      = now >> 4;
    gCycleHue     = now / 40;

    // PULSE: berdenyut mengikuti beat asli; saat hening pakai denyut default.
    if (analyzerIdle())
        gPulseBri = beatsin8(50, 100, 255);
    else
        gPulseBri = (uint8_t)(100.0f + 155.0f * beatPulse);

    gPulseHue = beatsin8(6, 0, 224);

    // PEAKHUE: hue mengikuti level audio (sudah ter-smooth di analyzer).
    float lv = overallLevel * 1.5f;
    if (lv > 1.0f) lv = 1.0f;
    if (lv < 0.0f) lv = 0.0f;

    gPeakHue = (uint8_t)(160.0f - lv * 160.0f);
}

CRGB dynColor(uint8_t type, CRGB solid, uint8_t pos, uint8_t bri)
{
    CRGB c;

    switch (type)
    {
        case CT_SOLID:   c = solid; break;
        case CT_RAINBOW: c = CHSV((uint8_t)(pos + gRainbowShift), 255, 255); break;
        case CT_PEAKHUE: c = CHSV((uint8_t)(gPeakHue + (pos >> 3)), 255, 255); break;
        case CT_FLOW:    c = CHSV((uint8_t)(gFlowHue + (pos >> 1)), 255, 255); break;
        case CT_PULSE:   c = CHSV((uint8_t)(gPulseHue + (pos >> 4)), 255, gPulseBri); break;
        case CT_CYCLE:   c = CHSV(gCycleHue, 255, 255); break;
        default:         c = CRGB(255, 255, 255); break;
    }

    c.nscale8_video(bri);
    return c;
}

CRGB colorFor(uint8_t m, uint8_t pos, uint8_t bri, CRGB orig)
{
    const ModeColor &mc = modeColor[m];

    if (mc.type == CT_ORIGINAL)
        return orig;

    return dynColor(mc.type, mc.solid, pos, bri);
}

CRGB tint(uint8_t m, CRGB orig, uint8_t pos)
{
    const ModeColor &mc = modeColor[m];

    if (mc.type == CT_ORIGINAL)
        return orig;

    uint8_t v = max(orig.r, max(orig.g, orig.b));

    return dynColor(mc.type, mc.solid, pos, v);
}

void recolorLit(uint8_t m)
{
    const ModeColor &mc = modeColor[m];

    if (mc.type == CT_ORIGINAL)
        return;

    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            CRGB &p = leds[XY(x, y)];

            uint8_t v = max(p.r, max(p.g, p.b));

            if (v)
                p = dynColor(mc.type, mc.solid, x * 16, v);
        }
    }
}

int parseMode(const String &name)
{
    for (int i = 0; i < MODE_TOTAL; i++)
    {
        if (name.equals(MODE_NAMES[i]))
            return i;
    }

    return -1;
}
