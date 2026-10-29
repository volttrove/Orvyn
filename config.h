#pragma once
#include <Arduino.h>

// ============================================================
//  AUDIO / FFT
// ============================================================
// Ukuran jendela FFT (HARUS pangkat 2). 512 @ 44.1 kHz = ~86 Hz per bin,
// cukup untuk memisahkan bass. Kalau FPS terasa turun, coba 256.
#define SAMPLES            512
#define HOP_SAMPLES        256      // minimal sampel baru sebelum FFT berikutnya
#define AUDIO_SAMPLE_RATE  44100
#define NUM_BANDS          16       // jumlah band logaritmik untuk visualizer

// ============================================================
//  LED MATRIX
// ============================================================
#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 16
#define NUM_LEDS      256
#define LED_PIN       23

// Batas arus LED (mA) pada 5V. SESUAIKAN dengan power supply kamu!
// 256 LED putih penuh bisa narik ~15A; tanpa batas ini ESP32 rawan brownout/reset.
#define LED_MAX_MILLIAMPS 2500

// Target waktu per frame (ms). 20 = ~50 FPS.
#define FRAME_MS 20

// Setelah berapa ms tanpa suara visualizer pindah ke animasi ambient.
#define IDLE_AFTER_MS 2500

// ============================================================
//  TOMBOL
// ============================================================
#define BTN_UP    27
#define BTN_DOWN  14
#define BTN_NEXT  32
#define BTN_PREV  33

// ============================================================
//  OLED
// ============================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const int METADATA_MAX_CHARS = 21;
const int METADATA_SCROLL_MS = 180;

enum VisualMode
{
    // VISIULIZER FFT
    MODE_WAVE = 0,
    MODE_SPECTRUM,
    MODE_VU,
    MODE_MIRROR,
    MODE_PEAK,
    MODE_CIRCLE,
    MODE_LINE,
    MODE_PLASMA,
    // ANIMASI
    MODE_RAIN,
    MODE_TWINKLE,
    MODE_LAVA,
    MODE_GLYPH,
    // TEXT
    MODE_TEXT,

    MODE_TOTAL
};

extern const char *const MODE_NAMES[MODE_TOTAL];
