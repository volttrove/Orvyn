#include "actions.h"
#include "audio.h"
#include "state.h"
#include "settings.h"

void actionPlay()
{
    Serial.println("PLAY");

    if (!a2dpConnected)
        return;

    a2dp_sink.play();
    isPlaying = true;
}

void actionPause()
{
    Serial.println("PAUSE");

    if (!a2dpConnected)
        return;

    a2dp_sink.pause();
    isPlaying = false;
}

void actionNext()
{
    Serial.println("NEXT SONG");

    if (a2dpConnected)
        a2dp_sink.next();
}

void actionPrev()
{
    Serial.println("PREVIOUS SONG");

    if (a2dpConnected)
        a2dp_sink.previous();
}

void actionMatrix(bool on)
{
    matrixEnabled = on;

    Serial.print("MATRIX: ");
    Serial.println(on ? "ON" : "OFF");

    if (!on)
    {
        FastLED.clear();
        FastLED.show();
    }
}

void actionBrightness(int value)
{
    value = constrain(value, 0, 255);

    if (value == brightness)
        return;

    brightness = value;
    FastLED.setBrightness(brightness);
    settingsMarkDirty();

    Serial.print("Brightness: ");
    Serial.println(brightness);
}

void actionSetMode(int mode)
{
    if (mode < 0 || mode >= MODE_TOTAL)
        return;

    visualizerMode = (VisualMode)mode;
    settingsMarkDirty();

    Serial.print("Mode: ");
    Serial.println(MODE_NAMES[mode]);
}

void actionStepMode(int dir)
{
    int m = (int)visualizerMode + dir;

    if (m < 0)           m = MODE_TOTAL - 1;
    if (m >= MODE_TOTAL) m = 0;

    actionSetMode(m);
}
