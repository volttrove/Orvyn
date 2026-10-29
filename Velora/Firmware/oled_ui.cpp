#include "oled_ui.h"
#include "state.h"
#include "audio.h"
#include <Wire.h>
#include <string.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static const unsigned long ABOUT_DURATION_MS = 10000;
static const uint32_t OLED_REFRESH_MS = 100;

static const char *const MODE_LABELS[MODE_TOTAL] =
{
    "Wave", "Spectrum", "VU", "Mirror", "Peak", "Circle", "Line",
    "Plasma", "Rain", "Twinkle", "Lava", "Glyph", "Text"
};

// ------------------------------------------------------------
// Helper teks (tanpa String -> tidak ada fragmentasi heap)
// ------------------------------------------------------------

// Ambil jendela METADATA_MAX_CHARS karakter dari teks yang di-scroll.
static void scrollWindow(const char *text, int &pos, char *out)
{
    int len = (int)strlen(text);

    if (len <= METADATA_MAX_CHARS)
    {
        strcpy(out, text);
        pos = 0;
        return;
    }

    int cycle = len + 5;                 // teks + 5 spasi pemisah

    if (pos >= cycle)
        pos = 0;

    for (int i = 0; i < METADATA_MAX_CHARS; i++)
    {
        int idx = (pos + i) % cycle;
        out[i] = (idx < len) ? text[idx] : ' ';
    }

    out[METADATA_MAX_CHARS] = '\0';
}

// Font default 5x7 = 6 piksel per karakter pada textSize 1.
static void printCentered(const char *txt, int y)
{
    int w = (int)strlen(txt) * 6;
    display.setCursor((SCREEN_WIDTH - w) / 2, y);
    display.print(txt);
}

static void drawAboutScreen()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor((SCREEN_WIDTH - 6 * 12) / 2, 0);
    display.print("VELORA");

    display.setTextSize(1);
    printCentered("Smart Audio System", 18);
    printCentered("Build 7 Juli 2026", 28);
    printCentered("ESP32 WROOM-32", 38);
    printCentered("By DMMY VOLTTROVE", 48);
    printCentered("Orvyn Inside", 56);
}

static void drawMain(const char *titleText, const char *artistText)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // HEADER
    display.setCursor(9, 1);
    if (a2dpConnected)
        display.fillCircle(2, 5, 2, SSD1306_WHITE);

    display.print("Orvyn");

    if (!a2dpConnected && audioMicActive())
    {
        display.setCursor(64, 1);
        display.print("MIC");
    }

    if (a2dpConnected)
    {
        if (isPlaying)
        {
            display.fillTriangle(119, 1, 119, 10, 127, 5, SSD1306_WHITE);
        }
        else
        {
            display.fillRect(119, 1, 3, 9, SSD1306_WHITE);
            display.fillRect(125, 1, 3, 9, SSD1306_WHITE);
        }
    }

    // TITLE + ARTIST
    printCentered(titleText, 21);
    printCentered(artistText, 35);

    // FOOTER
    display.setCursor(0, 55);
    display.print(MODE_LABELS[visualizerMode]);

    char bri[16];
    snprintf(bri, sizeof(bri), "LED: %d%%", (int)map(brightness, 0, 255, 0, 100));

    int w = (int)strlen(bri) * 6;
    display.setCursor(SCREEN_WIDTH - w, 55);
    display.print(bri);
}

static void oledTask(void *param)
{
    char title[META_LEN];
    char artist[META_LEN];
    char titleWin[METADATA_MAX_CHARS + 1];
    char artistWin[METADATA_MAX_CHARS + 1];

    uint32_t version = 0, seenVersion = 0;
    int titlePos = 0, artistPos = 0;
    unsigned long lastScroll = 0;

    for (;;)
    {
        if (metaGet(title, sizeof(title), artist, sizeof(artist), &version))
        {
            if (version != seenVersion)
            {
                seenVersion = version;
                titlePos = 0;
                artistPos = 0;
            }
        }

        if (millis() - lastScroll >= (unsigned long)METADATA_SCROLL_MS)
        {
            lastScroll = millis();

            if (strlen(title) > (size_t)METADATA_MAX_CHARS)  titlePos++;
            if (strlen(artist) > (size_t)METADATA_MAX_CHARS) artistPos++;
        }

        scrollWindow(title, titlePos, titleWin);
        scrollWindow(artist, artistPos, artistWin);

        drawMain(titleWin, artistWin);

        if (aboutActive)
        {
            if (millis() - aboutStart >= ABOUT_DURATION_MS)
                aboutActive = false;
            else
                drawAboutScreen();
        }

        display.display();

        vTaskDelay(pdMS_TO_TICKS(OLED_REFRESH_MS));
    }
}

void oledInit()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED: tidak terdeteksi (alamat 0x3C), task OLED tidak dijalankan");
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);

    display.setCursor(10, 20);
    display.println("VELORA");

    display.display();

    delay(1000);

    // Setelah titik ini HANYA task OLED yang menyentuh `display` & Wire.
    xTaskCreatePinnedToCore(oledTask, "oled", 6144, NULL, 1, NULL, 0);
}
