#include "Buzzer.h"
#include "Config.h"
#include <Arduino.h>

// Active-high. Pakai transistor driver untuk buzzer 5V, jangan langsung dari GPIO.
static bool active = false;
static uint32_t offAt = 0;

void buzzerBegin() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

void buzzerBeep() {
  digitalWrite(PIN_BUZZER, HIGH);
  active = true;
  offAt = millis() + BUZZER_PULSE_MS;
}

void buzzerUpdate() {
  if (active && (int32_t)(millis() - offAt) >= 0) {
    digitalWrite(PIN_BUZZER, LOW);
    active = false;
  }
}
