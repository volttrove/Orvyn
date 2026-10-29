#pragma once
#include <Arduino.h>

void actionPlay();
void actionPause();
void actionNext();
void actionPrev();
void actionMatrix(bool on);

void actionBrightness(int value);     // 0..255, langsung diterapkan & disimpan
void actionSetMode(int mode);         // 0..MODE_TOTAL-1
void actionStepMode(int dir);         // +1 / -1, berputar
