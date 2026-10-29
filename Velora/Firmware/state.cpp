#include "state.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

const char *const MODE_NAMES[MODE_TOTAL] =
{
    "WAVE", "SPECTRUM", "VU", "MIRROR", "PEAK", "CIRCLE", "LINE",
    "PLASMA", "RAIN", "TWINKLE", "LAVA", "GLYPH", "TEXT"
};

String runningText = "Orvyn";

volatile int level = 0;

int brightness = 80;                 // sama dengan nilai awal FastLED di setup()
bool matrixEnabled = true;
bool isPlaying = false;
VisualMode visualizerMode = MODE_WAVE;
bool bleConnected = false;
volatile bool a2dpConnected = false;
bool aboutActive = false;
unsigned long aboutStart = 0;

CRGB leds[NUM_LEDS];

uint16_t XY(uint8_t x, uint8_t y)
{
    x = MATRIX_WIDTH - 1 - x;
    y = MATRIX_HEIGHT - 1 - y;

    if (y & 1)
        return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
    else
        return y * MATRIX_WIDTH + x;
}

// ============================================================
//  METADATA (thread-safe)
// ============================================================
static SemaphoreHandle_t metaMutex = NULL;
static char metaTitle[META_LEN]  = "No song";
static char metaArtist[META_LEN] = "";
static volatile uint32_t metaVersion = 1;

// Salin hanya ASCII yang bisa ditampilkan font OLED. Karakter multibyte
// (mis. huruf Jepang) diganti satu '?' supaya tidak jadi karakter acak.
static void sanitizeAscii(char *dst, size_t n, const char *src)
{
    size_t o = 0;
    bool lastQ = false;

    for (const uint8_t *p = (const uint8_t *)src; *p && o + 1 < n; ++p)
    {
        uint8_t b = *p;

        if (b >= 32 && b < 127)
        {
            dst[o++] = (char)b;
            lastQ = false;
        }
        else if (b >= 0xC0)              // byte awal karakter multibyte
        {
            if (!lastQ)
            {
                dst[o++] = '?';
                lastQ = true;
            }
        }
        // byte lanjutan (0x80-0xBF) & karakter kontrol dilewati
    }

    dst[o] = '\0';
}

void metaInit()
{
    if (metaMutex == NULL)
        metaMutex = xSemaphoreCreateMutex();
}

void metaSet(MetaField field, const char *text)
{
    if (metaMutex == NULL)
        return;

    char tmp[META_LEN];
    sanitizeAscii(tmp, sizeof(tmp), text ? text : "");

    if (xSemaphoreTake(metaMutex, pdMS_TO_TICKS(20)) != pdTRUE)
        return;

    char *dst = (field == META_TITLE) ? metaTitle : metaArtist;

    if (strcmp(dst, tmp) != 0)
    {
        strncpy(dst, tmp, META_LEN - 1);
        dst[META_LEN - 1] = '\0';
        metaVersion = metaVersion + 1;
    }

    xSemaphoreGive(metaMutex);
}

bool metaGet(char *title, size_t titleSize,
             char *artistOut, size_t artistSize,
             uint32_t *version)
{
    if (metaMutex == NULL)
        return false;

    if (xSemaphoreTake(metaMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return false;

    strncpy(title, metaTitle, titleSize - 1);
    title[titleSize - 1] = '\0';

    strncpy(artistOut, metaArtist, artistSize - 1);
    artistOut[artistSize - 1] = '\0';

    if (version)
        *version = metaVersion;

    xSemaphoreGive(metaMutex);
    return true;
}
