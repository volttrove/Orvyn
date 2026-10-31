#include "Buttons.h"
#include "Config.h"
#include "Relays.h"
#include "Settings.h"
#include "Buzzer.h"
#include <Arduino.h>

struct Button {
  uint8_t pin;
  uint8_t channel;
  bool stable;
  bool lastReading;
  uint32_t debounceAt;
  uint32_t pressStart;
  bool longHandled;
};

static Button buttons[2] = {
  {PIN_BUTTON_1, CH_FAN, true, true, 0, 0, false},
  {PIN_BUTTON_2, CH_LAMP, true, true, 0, 0, false},
};

static void onShortPress(uint8_t id) {
  bool wasAuto = relaySetManual(id, !relayState(id));
  if (wasAuto) buzzerBeep();
  Serial.printf("BUTTON R%u -> %s (MANUAL)\n", (unsigned)(id + 1), relayState(id) ? "ON" : "OFF");
}

static void onLongPress(uint8_t id) {
  if (id == CH_LAMP && !settings.lampSet) {
    Serial.println("BUTTON R2 AUTO ERROR: NO SAVED SCHEDULE");
    return;
  }
  bool wasAuto = relayIsAuto(id);
  if (relayEnableAuto(id) && !wasAuto) buzzerBeep();
  Serial.printf("BUTTON R%u -> AUTO\n", (unsigned)(id + 1));
}

static void updateButton(Button &b) {
  uint32_t t = millis();
  bool reading = digitalRead(b.pin);

  if (reading != b.lastReading) {
    b.debounceAt = t;
    b.lastReading = reading;
  }

  if (t - b.debounceAt >= BUTTON_DEBOUNCE_MS && reading != b.stable) {
    b.stable = reading;
    if (b.stable == LOW) {
      b.pressStart = t;
      b.longHandled = false;
    } else if (!b.longHandled) {
      onShortPress(b.channel);  // short press diproses saat dilepas
    }
  }

  if (b.stable == LOW && !b.longHandled && t - b.pressStart >= BUTTON_LONG_PRESS_MS) {
    b.longHandled = true;
    onLongPress(b.channel);
  }
}

void buttonsBegin() {
  for (auto &b : buttons) pinMode(b.pin, INPUT_PULLUP);
}

void buttonsUpdate() {
  for (auto &b : buttons) updateButton(b);
}
