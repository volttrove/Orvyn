#include "buttons.h"
#include "state.h"
#include "actions.h"

// DOWN : short = lagu sebelumnya,  long = animasi sebelumnya
// UP   : short = lagu berikutnya,  long = animasi berikutnya
// NEXT : short = play/pause,       long (tahan) = brightness +
// PREV : short = matrix on/off,    long (tahan) = brightness -
//
// Semua tombol di-debounce. Brightness naik/turun berulang selama ditahan.

enum { B_UP = 0, B_DOWN, B_NEXT, B_PREV, B_COUNT };

struct Button
{
    uint8_t pin;
    bool    raw;            // pembacaan terakhir (belum stabil)
    bool    stable;         // status stabil setelah debounce (HIGH = lepas)
    uint32_t rawChangedMs;
    uint32_t pressedMs;
    uint32_t lastRepeatMs;
    bool    longFired;
};

static Button btn[B_COUNT] =
{
    { BTN_UP,   HIGH, HIGH, 0, 0, 0, false },
    { BTN_DOWN, HIGH, HIGH, 0, 0, 0, false },
    { BTN_NEXT, HIGH, HIGH, 0, 0, 0, false },
    { BTN_PREV, HIGH, HIGH, 0, 0, 0, false },
};

static const uint32_t DEBOUNCE_MS = 25;
static const uint32_t LONG_PRESS_MS = 700;
static const uint32_t REPEAT_MS = 160;
static const int BRIGHTNESS_STEP = 16;

void buttonsInit()
{
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(BTN_PREV, INPUT_PULLUP);
}

static bool isRepeatable(int i)
{
    return i == B_NEXT || i == B_PREV;
}

static void onShort(int i)
{
    switch (i)
    {
        case B_DOWN: actionPrev(); break;
        case B_UP:   actionNext(); break;

        case B_NEXT:
            if (isPlaying) actionPause();
            else           actionPlay();
            break;

        case B_PREV:
            actionMatrix(!matrixEnabled);
            break;
    }
}

static void onLong(int i)
{
    switch (i)
    {
        case B_DOWN: actionStepMode(-1); break;
        case B_UP:   actionStepMode(+1); break;
        case B_NEXT: actionBrightness(brightness + BRIGHTNESS_STEP); break;
        case B_PREV: actionBrightness(brightness - BRIGHTNESS_STEP); break;
    }
}

void readButtons()
{
    uint32_t now = millis();

    for (int i = 0; i < B_COUNT; i++)
    {
        Button &b = btn[i];

        bool r = digitalRead(b.pin);

        if (r != b.raw)
        {
            b.raw = r;
            b.rawChangedMs = now;
        }

        // Status berubah hanya kalau sudah stabil >= DEBOUNCE_MS
        if (b.raw != b.stable && now - b.rawChangedMs >= DEBOUNCE_MS)
        {
            b.stable = b.raw;

            if (b.stable == LOW)            // ditekan
            {
                b.pressedMs = now;
                b.longFired = false;
            }
            else                            // dilepas
            {
                if (!b.longFired)
                    onShort(i);
            }
        }

        // Sedang ditahan
        if (b.stable == LOW)
        {
            if (!b.longFired && now - b.pressedMs >= LONG_PRESS_MS)
            {
                b.longFired = true;
                b.lastRepeatMs = now;
                onLong(i);
            }
            else if (b.longFired && isRepeatable(i) &&
                     now - b.lastRepeatMs >= REPEAT_MS)
            {
                b.lastRepeatMs = now;
                onLong(i);
            }
        }
    }
}
