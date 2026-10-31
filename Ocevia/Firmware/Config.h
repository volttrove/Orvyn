#pragma once
#include <Arduino.h>

// =====================================================
// OCEVIA - ESP32-C3
// Semua konstanta & pengaturan hardware ada di sini.
// =====================================================

// ---------- Pin ----------
constexpr uint8_t PIN_DS18B20 = 4;
constexpr uint8_t PIN_BUTTON_1 = 6;
constexpr uint8_t PIN_BUTTON_2 = 7;
constexpr uint8_t PIN_LCD_SDA = 8;
constexpr uint8_t PIN_LCD_SCL = 9;
constexpr uint8_t PIN_RELAY_1 = 3;   // kipas
constexpr uint8_t PIN_RELAY_2 = 10;  // lampu
constexpr uint8_t PIN_BUZZER = 5;

constexpr bool RELAY_ACTIVE_LOW = true;
constexpr uint8_t LCD_ADDRESS = 0x27;

// ---------- BLE ----------
constexpr const char *BLE_DEVICE_NAME = "Ocevia Control";
constexpr const char *BLE_SERVICE_UUID = "12345678-1234-1234-1234-123456789000";
constexpr const char *BLE_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789001";
constexpr uint16_t BLE_MTU = 517;
constexpr size_t CMD_MAX_LEN = 64;
constexpr uint8_t CMD_QUEUE_LENGTH = 8;
constexpr uint32_t STATUS_NOTIFY_MS = 2000;  // notify periodik saat terkoneksi
constexpr uint32_t STATUS_MIN_GAP_MS = 100;  // jarak minimum antar notify

// ---------- Sensor suhu ----------
constexpr uint32_t TEMP_INTERVAL_MS = 1000;
constexpr uint32_t TEMP_CONVERSION_MS = 800;     // 12-bit DS18B20 butuh ~750 ms
constexpr uint32_t TEMP_STALE_MS = 10000;        // tanpa bacaan valid > ini = sensor error
constexpr uint32_t SENSOR_BOOT_GRACE_MS = 10000; // toleransi saat baru boot
constexpr uint32_t SENSOR_RESCAN_MS = 5000;
constexpr float TEMP_POWER_ON_VALUE = 85.0f;     // nilai default DS18B20 saat error/power-on
// Perilaku kipas AUTO kalau sensor error: true = kipas ON (aman untuk pendinginan)
constexpr bool FAN_FAILSAFE_ON = true;

// ---------- RTC ----------
constexpr uint32_t RTC_POLL_MS = 500;
constexpr uint32_t RTC_RETRY_MS = 5000;
constexpr uint16_t RTC_MIN_YEAR = 2020;
constexpr uint16_t RTC_MAX_YEAR = 2099;

// ---------- Relay ----------
constexpr uint32_t RELAY_EVAL_MS = 500;
constexpr uint32_t RELAY_MIN_SWITCH_MS = 30000;  // anti-chattering (hanya untuk AUTO)
constexpr uint32_t MAX_TIMER_SECONDS = 86400;    // maksimal 24 jam

// ---------- Default setting ----------
constexpr float FAN_ON_DEFAULT = 28.2f;
constexpr float FAN_OFF_DEFAULT = 27.5f;
constexpr uint16_t LAMP_ON_DEFAULT = 18 * 60;
constexpr uint16_t LAMP_OFF_DEFAULT = 22 * 60;

// ---------- Tombol ----------
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 1500;

// ---------- Buzzer ----------
constexpr uint32_t BUZZER_PULSE_MS = 100;

// ---------- LCD ----------
constexpr uint32_t LCD_REFRESH_MS = 250;
constexpr uint32_t ABOUT_DURATION_MS = 10000;
constexpr uint32_t ABOUT_PAGE_MS = 5000;

// ---------- Sistem ----------
constexpr uint16_t WIRE_TIMEOUT_MS = 50;
constexpr uint8_t WDT_TIMEOUT_S = 8;
constexpr uint32_t LOOP_DELAY_MS = 5;
