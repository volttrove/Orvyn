#pragma once
#include <Arduino.h>
#include "BluetoothA2DPSink.h"

extern BluetoothA2DPSink a2dp_sink;

void audioInit();
void audioProcess();       // panggil tiap loop(): ambil audio -> FFT -> analyzer
bool audioMicActive();     // true kalau INMP441 sedang jadi sumber visualizer
