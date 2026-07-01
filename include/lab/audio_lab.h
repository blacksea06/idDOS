#pragma once
#include <Arduino.h>

namespace AudioLab {

constexpr uint32_t WAV_SAMPLE_RATE = 16000;
constexpr uint16_t WAV_BITS        = 16;
constexpr uint8_t  WAV_CHANNELS    = 1;

bool begin();
bool available();
void idleSilence();

bool startRecord();
bool updateRecord();
bool recordingActive();
bool stopRecord();
void discardRecord();
bool saveRecordAs(const String& userName, String& outPath);
bool tempReady();
uint32_t recordMillis();
uint32_t recordBytes();
int16_t lastLevel();
int16_t readLiveLevel();

bool playerOpen(const String& path);
void playerClose();
bool playerStart();
void playerStop();
void playerUpdate();
bool playerPlaying();
String playerName();
uint32_t playerPositionMs();
uint32_t playerDurationMs();
void playerSetVolume(uint8_t volume);
uint8_t playerVolume();

String normalizeWavName(const String& userName);

} // namespace AudioLab
