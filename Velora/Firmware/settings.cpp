#include "settings.h"
#include "state.h"
#include "color_engine.h"
#include <Preferences.h>

static Preferences prefs;
static bool dirty = false;
static uint32_t dirtyMs = 0;

static const uint32_t SAVE_DELAY_MS = 3000;
static const int MIN_BOOT_BRIGHTNESS = 8;   // hindari boot dengan LED mati total

void settingsLoad()
{
    if (!prefs.begin("Orvyn", false))
    {
        Serial.println("NVS: gagal dibuka, pakai nilai default");
        return;
    }

    int b = prefs.getUChar("bri", (uint8_t)brightness);
    if (b < MIN_BOOT_BRIGHTNESS) b = MIN_BOOT_BRIGHTNESS;
    brightness = b;

    uint8_t m = prefs.getUChar("mode", (uint8_t)visualizerMode);
    if (m < MODE_TOTAL)
        visualizerMode = (VisualMode)m;

    size_t sz = prefs.getBytesLength("mc");
    if (sz == sizeof(modeColor))
    {
        prefs.getBytes("mc", modeColor, sz);

        for (int i = 0; i < MODE_TOTAL; i++)
            if (modeColor[i].type > CT_CYCLE)
                modeColor[i].type = CT_ORIGINAL;
    }

    String t = prefs.getString("txt", "");
    if (t.length() > 0)
        runningText = t;

    prefs.end();

    Serial.printf("NVS: bri=%d mode=%s\n", brightness, MODE_NAMES[visualizerMode]);
}

void settingsMarkDirty()
{
    dirty = true;
    dirtyMs = millis();
}

void settingsLoop()
{
    if (!dirty || millis() - dirtyMs < SAVE_DELAY_MS)
        return;

    dirty = false;

    if (!prefs.begin("Orvyn", false))
        return;

    prefs.putUChar("bri", (uint8_t)brightness);
    prefs.putUChar("mode", (uint8_t)visualizerMode);
    prefs.putBytes("mc", modeColor, sizeof(modeColor));
    prefs.putString("txt", runningText);
    prefs.end();

    Serial.println("NVS: setting tersimpan");
}
