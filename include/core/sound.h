#pragma once
#include <Arduino.h>

void soundBegin();
void soundSetEnabled(bool enabled);
bool soundIsEnabled();
void soundSetVolume(uint8_t volume);

void soundBoot();
void soundBootChime();
void soundSuccess();
void soundError();
void soundSave();
void soundRun();
void soundTest();
void soundSilence();
