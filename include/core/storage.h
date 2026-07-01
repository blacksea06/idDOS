#pragma once
#include <Arduino.h>

bool storageInit();
bool storageReady();

bool storageCheckLayout();
bool storageCreateLayout();

uint64_t storageTotalBytes();
uint64_t storageUsedBytes();

const char* storageAudioDir();
