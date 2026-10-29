#include "visualizers.h"
#include "config.h"
#include "state.h"
#include "analyzer.h"
#include "color_engine.h"
#include "font5x7.h"
#include "fontJP16x16.h"
#include <math.h>

static void drawWave();
static void drawSpectrum();
static void drawVU();
static void drawMirrorSpectrum();
static void drawPeakSpectrum();
static void drawCirclePulse();
static void drawLineSpectrum();
static void drawPlasma();
static void drawRain();
static void drawTwinkle();
static void drawLava();
static void drawGlyph();

// Delta waktu frame (detik). Semua gerakan memakai ini, bukan "per frame",
// sehingga kecepatan animasi tidak berubah walau FPS naik-turun.
static float gDt = 0.02f;

// ------------------------------------------------------------
// Helper
// ------------------------------------------------------------
static inline int rowsFor(float v)
{
    int h = (int)(v * MATRIX_HEIGHT + 0.5f);

    if (h < 0) h = 0;
    if (h > MATRIX_HEIGHT) h = MATRIX_HEIGHT;

    return h;
}

// 16 band -> 8 kolom (ambil yang terbesar dari tiap pasangan)
static inline float band8(int i)
{
    return fmaxf(spectrum[i * 2], spectrum[i * 2 + 1]);
}

static inline CRGB heightColor(uint8_t mode, int y)
{
    return colorFor(mode, (uint8_t)(y * 16), 255,
                    CHSV(map(y, 0, MATRIX_HEIGHT - 1, 96, 0), 255, 255));
}

static inline bool modeIsReactive(VisualMode m)
{
    return m <= MODE_PLASMA;      // WAVE .. PLASMA bergantung pada audio
}

// ------------------------------------------------------------
// Ambient: tampil halus saat tidak ada suara, memudar saat musik mulai
// ------------------------------------------------------------
static float idleMix = 0.0f;

static void updateIdleMix()
{
    if (analyzerIdle())
        idleMix = fminf(1.0f, idleMix + gDt * 0.6f);     // masuk perlahan (~1.7 dtk)
    else
        idleMix = fmaxf(0.0f, idleMix - gDt * 4.0f);     // hilang cepat saat musik mulai
}

static void addAmbient(float mix)
{
    static float tf = 0.0f;

    tf += gDt * 18.0f;
    if (tf > 1.0e7f) tf = 0.0f;

    uint16_t t = (uint16_t)((uint32_t)tf);
    uint8_t m8 = (uint8_t)(mix * 255.0f);

    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            uint8_t n = inoise8(x * 28, y * 28, t);

            uint8_t v = scale8(qsub8(n, 60), 110);
            v = scale8(v, m8);

            if (v)
            {
                CRGB base = CHSV(135 + (n >> 2), 230, v);
                leds[XY(x, y)] += tint(visualizerMode, base, (x + y) * 8);
            }
        }
    }
}

// ------------------------------------------------------------
// Render utama
// ------------------------------------------------------------
void renderMatrix()
{
    uint32_t now = millis();

    static uint32_t lastFrame = 0;
    float dt = (lastFrame == 0) ? 0.02f : (now - lastFrame) / 1000.0f;
    lastFrame = now;

    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.1f)   dt = 0.1f;
    gDt = dt;

    if (!matrixEnabled)
    {
        // Cukup refresh sesekali (menghapus glitch akibat noise), bukan tiap frame.
        static uint32_t lastBlank = 0;

        if (now - lastBlank > 500)
        {
            lastBlank = now;
            FastLED.clear();
            FastLED.show();
        }

        return;
    }

    // Setiap mode menggambar dari nol; state animasi disimpan di dalam mode masing-masing.
    FastLED.clear();

    switch (visualizerMode)
    {
        case MODE_WAVE:     drawWave();           break;
        case MODE_SPECTRUM: drawSpectrum();       break;
        case MODE_VU:       drawVU();             break;
        case MODE_MIRROR:   drawMirrorSpectrum(); break;
        case MODE_PEAK:     drawPeakSpectrum();   break;
        case MODE_CIRCLE:   drawCirclePulse();    break;
        case MODE_LINE:     drawLineSpectrum();   break;
        case MODE_PLASMA:   drawPlasma();         break;
        case MODE_RAIN:     drawRain();           break;
        case MODE_TWINKLE:  drawTwinkle();        break;
        case MODE_LAVA:     drawLava();           break;
        case MODE_GLYPH:    drawGlyph();          break;

        case MODE_TEXT:

            drawRunningText(runningText.c_str(), CRGB::White);
            recolorLit(MODE_TEXT);
            break;

        default: break;
    }

    if (modeIsReactive(visualizerMode))
    {
        updateIdleMix();

        if (idleMix > 0.01f)
            addAmbient(idleMix);
    }

    FastLED.show();
}

// ------------------------------------------------------------
// MODE: WAVE  (16 band sebagai garis)
// ------------------------------------------------------------
static void drawWave()
{
    int prevY = 0;

    for (int x = 0; x < MATRIX_WIDTH; x++)
    {
        int y = (int)((1.0f - spectrum[x]) * (MATRIX_HEIGHT - 1) + 0.5f);
        y = constrain(y, 0, MATRIX_HEIGHT - 1);

        // penghubung antar titik (digambar dulu supaya titik utama di atasnya)
        if (x > 0)
        {
            for (int yy = min(prevY, y); yy <= max(prevY, y); yy++)
                leds[XY(x, yy)] = colorFor(MODE_WAVE, x * 16, 150, CRGB(CRGB::Blue));
        }

        leds[XY(x, y)] = colorFor(MODE_WAVE, x * 16, 255, CRGB(CRGB::Cyan));

        prevY = y;
    }
}

// ------------------------------------------------------------
// MODE: SPECTRUM  (8 bar, lebar 2 kolom)
// ------------------------------------------------------------
static void drawSpectrum()
{
    for (int band = 0; band < 8; band++)
    {
        int h = rowsFor(band8(band));

        for (int y = 0; y < h; y++)
        {
            CRGB c = heightColor(MODE_SPECTRUM, y);

            for (int w = 0; w < 2; w++)
                leds[XY(band * 2 + w, MATRIX_HEIGHT - 1 - y)] = c;
        }
    }
}

// ------------------------------------------------------------
// MODE: VU  (level keseluruhan + penanda puncak)
// ------------------------------------------------------------
static void drawVU()
{
    static float peak = 0.0f;

    // Kurangi sensitivitas supaya bar tidak cepat penuh
    float v = overallLevel * 1.0f;

    if (v > 1.0f) v = 1.0f;
    if (v < 0.0f) v = 0.0f;

    // Peak turun perlahan
    peak = fmaxf(v, peak - gDt * 0.8f);

    int h = rowsFor(v);

    for (int y = 0; y < h; y++)
    {
        CRGB c = heightColor(MODE_VU, y);

        for (int x = 0; x < MATRIX_WIDTH; x++)
            leds[XY(x, MATRIX_HEIGHT - 1 - y)] = c;
    }
}

// ------------------------------------------------------------
// MODE: MIRROR  (bass di tengah, treble di tepi)
// ------------------------------------------------------------
static void drawMirrorSpectrum()
{
    for (int band = 0; band < 8; band++)
    {
        int h = rowsFor(band8(band));

        for (int y = 0; y < h; y++)
        {
            CRGB color = heightColor(MODE_MIRROR, y);

            leds[XY(8 + band, MATRIX_HEIGHT - 1 - y)] = color;   // kanan
            leds[XY(7 - band, MATRIX_HEIGHT - 1 - y)] = color;   // kiri (mirror)
        }
    }
}

// ------------------------------------------------------------
// MODE: PEAK  (16 bar + penanda puncak yang jatuh)
// ------------------------------------------------------------
static void drawPeakSpectrum()
{
    static float peak[MATRIX_WIDTH] = {0};

    for (int band = 0; band < MATRIX_WIDTH; band++)
    {
        float v = spectrum[band];
        int h = rowsFor(v);

        // puncak: naik instan, jatuh ~7 baris/detik
        peak[band] = fmaxf((float)h, peak[band] - gDt * 7.0f);

        for (int y = 0; y < h; y++)
            leds[XY(band, MATRIX_HEIGHT - 1 - y)] = heightColor(MODE_PEAK, y);

        if (peak[band] >= 1.0f)
        {
            int capY = MATRIX_HEIGHT - 1 - (int)peak[band];

            if (capY < 0) capY = 0;

            leds[XY(band, capY)] = colorFor(MODE_PEAK, 255, 255, CRGB(CRGB::White));
        }
    }
}

// ------------------------------------------------------------
// MODE: CIRCLE  (radius = bass, cincin saat beat)
// ------------------------------------------------------------
static void drawCirclePulse()
{
    float r = 0.9f + bassEnergy * 6.0f + beatPulse * 1.6f;

    const float c = (MATRIX_WIDTH - 1) * 0.5f;

    for (int y = 0; y < MATRIX_HEIGHT; y++)
    {
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            float dx = x - c;
            float dy = y - c;

            float d = sqrtf(dx * dx + dy * dy);

            uint8_t pos = (uint8_t)fminf(d * 32.0f, 255.0f);

            if (d < r)
            {
                int fall = (int)(d / r * 90.0f);          // sedikit lebih redup ke tepi
                uint8_t bri = (uint8_t)(255 - fall);

                leds[XY(x, y)] =
                    colorFor(MODE_CIRCLE, pos, bri,
                             CHSV((uint8_t)(d * 20.0f), 255, bri));
            }
            else if (beatPulse > 0.25f && d < r + 1.0f)
            {
                uint8_t bri = (uint8_t)(beatPulse * 110.0f);

                leds[XY(x, y)] =
                    colorFor(MODE_CIRCLE, pos, bri,
                             CHSV((uint8_t)(d * 20.0f), 255, bri));
            }
        }
    }
}

// ------------------------------------------------------------
// MODE: LINE  (spektrum simetris + jejak "ghost" yang meluruh)
// ------------------------------------------------------------
static void drawLineSpectrum()
{
    static float ghost[MATRIX_WIDTH] = {0};

    const int HALF = MATRIX_HEIGHT / 2;

    for (int x = 0; x < MATRIX_WIDTH; x++)
    {
        float v = spectrum[x];

        ghost[x] = fmaxf(v, ghost[x] - gDt * 0.9f);

        int h  = (int)(v * HALF + 0.5f);
        int gh = (int)(ghost[x] * HALF + 0.5f);

        uint8_t hue = map(x, 0, MATRIX_WIDTH - 1, 220, 0);

        for (int i = 0; i < HALF; i++)
        {
            uint8_t bri = 255 - i * 18;
            CRGB c;

            if (i < h)
                c = colorFor(MODE_LINE, x * 16, bri, CHSV(hue, 255, bri));
            else if (i < gh)
                c = colorFor(MODE_LINE, x * 16, bri >> 2, CHSV(hue, 255, bri >> 2));
            else
                break;

            leds[XY(x, HALF - 1 - i)] += c;     // atas
            leds[XY(x, HALF + i)]     += c;     // bawah
        }

        // garis tengah tipis
        CRGB base = colorFor(MODE_LINE, x * 16, 35, CHSV(160, 180, 35));
        leds[XY(x, HALF - 1)] += base;
        leds[XY(x, HALF)]     += base;
    }
}

// ------------------------------------------------------------
// MODE: PLASMA  (api yang disulut bass, flash saat beat)
// ------------------------------------------------------------
static uint8_t plasmaField[MATRIX_WIDTH][MATRIX_HEIGHT];
static uint8_t plasmaNext[MATRIX_WIDTH][MATRIX_HEIGHT];

// Satu langkah fisika plasma. Dijalankan pada ~25 Hz tetap supaya
// konstanta (decay/difusi) tetap terasa sama berapapun FPS-nya.
static void plasmaStep(int bass)
{
    bool beatHigh = (beatPulse > 0.7f);

    if (bass > 10)
    {
        int amount = map(bass, 10, 255, 15, 200);
        amount = constrain(amount, 15, 200);

        if (beatPulse > 0.3f)
        {
            amount += (int)(beatPulse * 120.0f);
            amount = constrain(amount, 0, 255);
        }

        int particles = (bass > 170) ? 5 : 2;

        if (beatHigh)
            particles += 3;

        for (int i = 0; i < particles; i++)
        {
            int x = random(3, MATRIX_WIDTH - 3);
            int y = random(3, MATRIX_HEIGHT - 3);

            plasmaField[x][y]     = qadd8(plasmaField[x][y], amount);
            plasmaField[x - 1][y] = qadd8(plasmaField[x - 1][y], amount / 3);
            plasmaField[x + 1][y] = qadd8(plasmaField[x + 1][y], amount / 3);
            plasmaField[x][y - 1] = qadd8(plasmaField[x][y - 1], amount / 3);
            plasmaField[x][y + 1] = qadd8(plasmaField[x][y + 1], amount / 3);
        }
    }

    for (int y = 1; y < MATRIX_HEIGHT - 1; y++)
    {
        for (int x = 1; x < MATRIX_WIDTH - 1; x++)
        {
            uint16_t average =
                plasmaField[x][y] * 3 +
                plasmaField[x - 1][y] + plasmaField[x + 1][y] +
                plasmaField[x][y - 1] + plasmaField[x][y + 1];

            average /= 7;

            plasmaNext[x][y] = qsub8(average, 3);   // plasma decay
        }
    }

    for (int y = 1; y < MATRIX_HEIGHT - 1; y++)
        for (int x = 1; x < MATRIX_WIDTH - 1; x++)
            plasmaField[x][y] = plasmaNext[x][y];
}

static void drawPlasma()
{
    static float tf = 0.0f;
    static float acc = 0.0f;

    float b = bassEnergy * 1.15f;
    if (b > 1.0f) b = 1.0f;

    uint8_t bass = (uint8_t)(b * 255.0f);
    uint8_t bp   = (uint8_t)(beatPulse * 255.0f);

    acc += gDt;

    int steps = 0;
    while (acc >= 0.04f && steps < 3)
    {
        plasmaStep(bass);
        acc -= 0.04f;
        steps++;
    }

    if (acc > 0.08f)
        acc = 0.0f;

    tf += gDt * 60.0f;
    if (tf > 1.0e7f) tf = 0.0f;

    uint16_t t = (uint16_t)((uint32_t)tf);

    for (int y = 0; y < MATRIX_HEIGHT; y++)
    {
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            uint8_t noise1 = inoise8(x * 65, y * 65, t);
            uint8_t noise2 = inoise8(x * 130 + 1000, y * 130 + 2000, (uint16_t)(t * 2));

            uint16_t plasma = noise1 * 3 + noise2;
            plasma /= 4;

            uint8_t value =
                qadd8(plasmaField[x][y], scale8((uint8_t)plasma, bass) / 5);

            uint8_t threshold = 45 + ((255 - bass) * 55 / 255);

            value = qsub8(value, threshold);

            if (value > 0)
                value = scale8(value, value);

            uint8_t red = 0, green = 0, blue = 0;

            if (value < 55)
            {
                red = scale8(value, 180);
            }
            else if (value < 120)
            {
                red = value;
                green = map(value, 55, 120, 0, 18);
            }
            else if (value < 185)
            {
                red = 255;
                green = map(value, 120, 185, 18, 90);
            }
            else if (value < 230)
            {
                red = 255;
                green = map(value, 185, 230, 90, 170);
            }
            else
            {
                red = 255;
                green = map(value, 230, 255, 170, 240);
                blue  = map(value, 230, 255, 0, 20);
            }

            if (bp > 60 && value > 100)
            {
                uint8_t flash = scale8(bp, 130);

                red   = qadd8(red, flash);
                green = qadd8(green, flash / 2);

                if (bp > 180 && value > 170)
                {
                    green = qadd8(green, 40);
                    blue  = qadd8(blue, 20);
                }
            }

            leds[XY(x, y)] =
                tint(MODE_PLASMA, CRGB(red, green, blue), (x + y) * 8);
        }
    }
}

// ------------------------------------------------------------
// MODE: RAIN 
// ------------------------------------------------------------
static void drawRain()
{
    static uint8_t xpos[18];
    static int8_t ypos[18];
    static uint8_t speed[18];
    static bool init = false;

    if (!init)
    {
        init = true;

        for (int i = 0; i < 18; i++)
        {
            xpos[i] = random(MATRIX_WIDTH);
            ypos[i] = random(MATRIX_HEIGHT);
            speed[i] = random(1, 3);
        }
    }

    fadeToBlackBy(leds, NUM_LEDS, 70);

    for (int i = 0; i < 18; i++)
    {
        uint8_t pos = xpos[i] * 16;

        if (ypos[i] >= 0 && ypos[i] < MATRIX_HEIGHT)
            leds[XY(xpos[i], ypos[i])] +=
                colorFor(MODE_RAIN, pos, 255, CRGB(180, 255, 255));

        if (ypos[i] > 0 && ypos[i] - 1 < MATRIX_HEIGHT)
            leds[XY(xpos[i], ypos[i] - 1)] +=
                colorFor(MODE_RAIN, pos, 150, CRGB(40, 120, 255));

        if (ypos[i] > 1 && ypos[i] - 2 < MATRIX_HEIGHT)
            leds[XY(xpos[i], ypos[i] - 2)] +=
                colorFor(MODE_RAIN, pos, 60, CRGB(10, 40, 120));

        ypos[i] += speed[i];

        if (ypos[i] >= MATRIX_HEIGHT + 2)
        {
            ypos[i] = -3;
            xpos[i] = random(MATRIX_WIDTH);
            speed[i] = random(1, 3);
        }
    }
}

// ------------------------------------------------------------
// MODE: TWINKLE  
// ------------------------------------------------------------
static void drawTwinkle()
{
    static uint16_t t = 0;
    t++;

    fadeToBlackBy(leds, NUM_LEDS, 4);

    if (random8() < 18)
    {
        uint16_t i = random16(NUM_LEDS);
        uint8_t p = random8();

        if (random8() < 200)
            leds[i] = colorFor(
                MODE_TWINKLE, p, 180, CRGB(180, 180, 180));
        else
            leds[i] = colorFor(
                MODE_TWINKLE, p, 255, CHSV(random8(145, 170), 40, 255));
    }

    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (leds[i])
        {
            uint8_t wave = cubicwave8((i * 11) + t * 2);
            leds[i].nscale8_video(wave);
        }
    }
}

// ------------------------------------------------------------
// MODE: LAVA  (aliran melaju mengikuti bass)
// ------------------------------------------------------------
static void drawLava()
{
    static float tf = 0.0f;

    tf += gDt * (35.0f + bassEnergy * 120.0f + beatPulse * 160.0f);
    if (tf > 1.0e7f) tf = 0.0f;

    uint16_t t = (uint16_t)((uint32_t)tf);

    uint8_t boost = (uint8_t)(bassEnergy * 40.0f + beatPulse * 60.0f);

    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            uint8_t n1 = inoise8(x * 25, y * 25, t);
            uint8_t n2 = inoise8(x * 35, y * 35, (uint16_t)(t * 2));

            uint8_t n = qadd8(scale8(n1, 120), scale8(n2, 120));

            n = qsub8(n, 100);
            n = qadd8(n, n);
            n = qadd8(n, boost);

            leds[XY(x, y)] =
                tint(MODE_LAVA,
                     ColorFromPalette(LavaColors_p, n, n),
                     (x + y) * 8);
        }
    }
}

// ------------------------------------------------------------
// MODE: GLYPH  
// ------------------------------------------------------------
static void drawGlyph()
{
    static uint8_t page = 0;
    static uint32_t last = 0;

    if (millis() - last > 1000)
    {
        last = millis();
        page++;

        if (page >= 4)
            page = 0;
    }

    CRGB glyphColor = CRGB(255, 80, 140);

    if (modeColor[MODE_GLYPH].type != CT_ORIGINAL)
        glyphColor = CRGB(255, 255, 255);

    switch (page)
    {
        case 0: drawGlyph16(FONT_YIN,  0, 0, glyphColor); break;
        case 1: drawGlyph16(FONT_GAKU, 0, 0, glyphColor); break;
        case 2: drawGlyph16(FONT_DEN,  0, 0, glyphColor); break;
        case 3: drawGlyph16(FONT_CHI,  0, 0, glyphColor); break;
    }

    recolorLit(MODE_GLYPH);
}

// ------------------------------------------------------------
// MODE: TEXT
// ------------------------------------------------------------
void drawRunningText(const char *text, CRGB color)
{
    static int offset = MATRIX_WIDTH;
    static unsigned long lastMove = 0;

    FastLED.clear();

    drawString(text, offset, 4, color);

    if (millis() - lastMove > 80)
    {
        offset--;
        lastMove = millis();
    }

    int textWidth = strlen(text) * 6;

    if (offset < -textWidth)
        offset = MATRIX_WIDTH;
}
