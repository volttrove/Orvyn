#include <Wire.h>
#include <esp_task_wdt.h>

#include "Config.h"
#include "Settings.h"
#include "Utils.h"
#include "Buzzer.h"
#include "Sensors.h"
#include "Relays.h"
#include "Buttons.h"
#include "Display.h"
#include "BleLink.h"
#include "Commands.h"

// =====================================================
// Ocevia - ESP32-C3 (versi modular)
// =====================================================

static void watchdogBegin() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.timeout_ms = WDT_TIMEOUT_S * 1000UL;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  if (esp_task_wdt_init(&cfg) == ESP_ERR_INVALID_STATE) esp_task_wdt_reconfigure(&cfg);
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);
}

static void printBanner() {
  char a[8], b[8];
  formatClock(settings.lampOn, a, sizeof(a));
  formatClock(settings.lampOff, b, sizeof(b));

  Serial.println();
  Serial.println("==============================");
  Serial.println("OCEVIA READY");
  Serial.println("==============================");
  Serial.printf("R1: %s (%s)\n", relayState(CH_FAN) ? "ON" : "OFF", relayIsAuto(CH_FAN) ? "AUTO" : "MANUAL");
  Serial.printf("R2: %s (%s)\n", relayState(CH_LAMP) ? "ON" : "OFF", relayIsAuto(CH_LAMP) ? "AUTO" : "MANUAL");
  Serial.printf("FAN TEMP: ON > %.1f C, OFF <= %.1f C\n", settings.fanOn, settings.fanOff);
  Serial.printf("LAMP: %s-%s%s\n", a, b, settings.lampSet ? "" : " (default, belum disimpan)");
  Serial.println("BLE COMMANDS:");
  Serial.println("R1 AUTO | R1 ON | R1 OFF | R1 3600");
  Serial.println("R1 TEMP 28.2,27.5 (ON,OFF thresholds)");
  Serial.println("R2:18,00:22,00 | R2 AUTO | R2 ON | R2 OFF | R2 3600");
  Serial.println("STATUS | ABOUT | SETTIME YYYY-MM-DD HH:MM:SS");
  Serial.println("BUTTON: short press=manual toggle; hold 1.5s=AUTO");
}

void setup() {
  Serial.begin(115200);

  relaysPreInit();  // pastikan relay OFF secepat mungkin
  buzzerBegin();

  Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL);
  Wire.setClock(100000);            // DS1307 maksimal 100 kHz
  Wire.setTimeOut(WIRE_TIMEOUT_MS); // cegah hang kalau kabel I2C goyang

  displayBegin();
  delay(1000);

  settingsLoad();
  sensorsBegin();
  buttonsBegin();

  relaysSetChangeCallback(bleRequestStatus);
  relaysBegin();

  bleBegin();
  printBanner();
  watchdogBegin();
}

void loop() {
  esp_task_wdt_reset();

  sensorsUpdate();
  buttonsUpdate();
  commandsUpdate();   // command BLE dieksekusi di sini, bukan di task BLE
  relaysUpdate();     // timer + AUTO
  buzzerUpdate();
  displayUpdate();
  bleUpdate();

  delay(LOOP_DELAY_MS);
}
