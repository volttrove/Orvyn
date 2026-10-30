/*
╔══════════════════════════════════════════════╗
║                VELORA                        ║
║              VOLTTROVE                       ║
╚══════════════════════════════════════════════╝

TARGET HARDWARE
- ESP32-WROOM-32
- Arduino Board: ESP32 Dev Module
- WS2812B Matrix: 16x16 / 256 LED
- PCM5102A DAC
- SSD1306 OLED 128x64 I2C
- INMP441

REQUIRED LIBRARIES
- ESP32-A2DP
- FastLED
- arduinoFFT (v2.x)
- Adafruit GFX Library
- Adafruit SSD1306
- Preferences (bawaan ESP32 core, tidak perlu install)

ARDUINO IDE
- Arduino IDE : 2.3.8
- ESP32 Boards : 2.0.18

IMPORTANT
Use ESP32-WROOM-32 / ESP32 Dev Module.
Other ESP32 variants may not be compatible.

MICROPHONE (fallback visualizer saat A2DP tidak tersambung)
INMP441 BCLK   = GPIO 18
INMP441 WS     = GPIO 5
INMP441 SD     = GPIO 4

POWER
Atur LED_MAX_MILLIAMPS di config.h sesuai power supply 5V kamu.

PINOUT
LED DATA       = GPIO 23
PCM5102 BCK    = GPIO 26
PCM5102 LRCK   = GPIO 25
PCM5102 DATA   = GPIO 19

BUTTONS
UP             = GPIO 27
DOWN           = GPIO 14
NEXT           = GPIO 32
PREV           = GPIO 33

OLED
Address        = 0x3C
SDA/SCL        = ESP32 Wire default

BLE
Device Name    = Velora Control

A2DP
Bluetooth Name = Velora

COMMANDS
PLAY
PAUSE
NEXT
PREV
ON:MATRIX
OF:MATRIX
ABOUT
T:TEXT
B:0-255
M:MODE
C:MODE:VALUE

BUILD NOTE
If WIDTH/HEIGHT conflicts with Adafruit_GFX,
use MATRIX_WIDTH/MATRIX_HEIGHT instead.

Project:
VELORA
Controller
https://volttrove.github.io/Orvyn/Orvyn.html
Maker:
VOLTTROVE
*/


#include <Wire.h>
#include <FastLED.h>
#include "config.h"
#include "state.h"
#include "settings.h"
#include "analyzer.h"
#include "color_engine.h"
#include "audio.h"
#include "ble_control.h"
#include "oled_ui.h"
#include "buttons.h"
#include "visualizers.h"
#include "font5x7.h"
#include "fontJP16x16.h"

void setup()
{
    Serial.begin(115200);

    buttonsInit();
    metaInit();              // mutex metadata (dipakai task Bluetooth & OLED)
    settingsLoad();          // brightness, mode, warna, teks dari NVS

    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
    FastLED.setBrightness(brightness);
    FastLED.clear(true);

    bleInit();
    oledInit();              // juga menjalankan task OLED
    audioInit();
}

void loop()
{
    uint32_t frameStart = millis();

    readButtons();
    processBLEQueue();
    settingsLoop();

    audioProcess();          // sumber audio -> FFT -> analyzer
    updateColorEngine();     // butuh hasil analyzer
    renderMatrix();          // gambar + FastLED.show()

    // Batasi frame rate. OLED berjalan di task sendiri, jadi tidak lagi
    // memblok loop ini.
    uint32_t spent = millis() - frameStart;
    delay(spent < FRAME_MS ? (FRAME_MS - spent) : 1);
}
