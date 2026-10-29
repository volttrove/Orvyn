#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

// Inisialisasi OLED + tampilkan splash, lalu menjalankan task FreeRTOS
// yang menggambar & mengirim layar. Dengan begitu transfer I2C (~25 ms)
// tidak lagi memblok animasi LED di loop().
void oledInit();
