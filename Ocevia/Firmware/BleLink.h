#pragma once
#include <Arduino.h>

void bleBegin();
void bleUpdate();                                   // notify status + restart advertising
void bleRequestStatus();                            // aman dipanggil dari mana saja
bool bleDequeueCommand(char *buf, size_t size);     // ambil command dari queue (dipanggil di loop)
