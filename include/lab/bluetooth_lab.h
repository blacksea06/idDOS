#pragma once
#include <Arduino.h>

void bleBegin();

bool bleServerStart();
void bleServerStop();      // soft stop: advertising off, state off
void bleFullStop();        // full NimBLE deinit and pointer reset
bool bleServerRunning();

bool bleAdvertisingStart();
void bleAdvertisingStop();
bool bleAdvertisingRunning();

void bleStopForWifi();     // full stop before WLAN

String bleDeviceName();
void bleSetDeviceName(const String& name);

String bleStatusText();
