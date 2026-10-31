#include "Display.h"
#include "Config.h"
#include "Relays.h"
#include "Sensors.h"
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

// Salinan isi LCD; 0 = tidak diketahui (paksa tulis ulang)
static char shadow[2][16];
static uint32_t lastUpdate = 0;

static bool aboutMode = false;
static uint32_t aboutStart = 0;
static int8_t aboutPage = -1;

static void invalidateShadow() {
  memset(shadow, 0, sizeof(shadow));
}

// Tulis hanya karakter yang berubah; setCursor hanya kalau tidak berurutan.
static void writeLine(uint8_t row, const char *text) {
  size_t len = strlen(text);
  int nextCol = -1;

  for (uint8_t i = 0; i < 16; i++) {
    char c = i < len ? text[i] : ' ';
    if (c == shadow[row][i]) continue;
    if (nextCol != i) lcd.setCursor(i, row);
    lcd.write((uint8_t)c);
    shadow[row][i] = c;
    nextCol = i + 1;
  }
}

static void drawAboutPage(int8_t page) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(page == 1 ? "ESP32-C3" : "Orvyn");
  lcd.setCursor(0, 1);
  lcd.print(page == 1 ? "Volttrove" : "23 AGUSTUS 2026");
}

void displayBegin() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("OCEVIA");
  lcd.setCursor(0, 1);
  lcd.print("CONTROLLER");
  invalidateShadow();
}

void displayShowAbout() {
  aboutMode = true;
  aboutStart = millis();
  aboutPage = 0;
  drawAboutPage(0);
}

static void updateAbout() {
  uint32_t elapsed = millis() - aboutStart;

  if (elapsed >= ABOUT_DURATION_MS) {
    aboutMode = false;
    lcd.clear();
    invalidateShadow();
    lastUpdate = 0;
    return;
  }

  int8_t page = elapsed >= ABOUT_PAGE_MS ? 1 : 0;
  if (page != aboutPage) {
    aboutPage = page;
    drawAboutPage(page);
  }
}

void displayUpdate() {
  if (aboutMode) {
    updateAbout();
    return;
  }

  uint32_t t = millis();
  if (t - lastUpdate < LCD_REFRESH_MS) return;
  lastUpdate = t;

  char tempStr[8];
  char timeStr[10];
  char line0[24];
  char line1[24];

  if (tempValid()) snprintf(tempStr, sizeof(tempStr), "%4.1f", tempValue());
  else strcpy(tempStr, "--.-");

  if (rtcOk()) {
    ClockTime c = rtcTime();
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u",
             (unsigned)c.hour, (unsigned)c.minute, (unsigned)c.second);
  } else {
    strcpy(timeStr, "--:--:--");
  }

  snprintf(line0, sizeof(line0), "%s%cC %s ", tempStr, (char)223, timeStr);
  snprintf(line1, sizeof(line1), "R1:%s%c R2:%s%c ",
           relayState(CH_FAN) ? "ON " : "OFF", relayIsAuto(CH_FAN) ? 'A' : 'M',
           relayState(CH_LAMP) ? "ON " : "OFF", relayIsAuto(CH_LAMP) ? 'A' : 'M');

  writeLine(0, line0);
  writeLine(1, line1);
}
