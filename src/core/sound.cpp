#include <M5Cardputer.h>
#include "sound.h"

static bool systemSoundEnabled = true;
static uint8_t systemVolume = 90;

void soundBegin()
{
    // Keep boot-audio path identical to the old known-good algorithm.
    // Do not set a high volume here: on Cardputer-Adv some builds produce
    // a loud pop / wrong startup sound when Speaker.begin() is followed by
    // an aggressive volume re-arm during cold boot.
    M5Cardputer.Speaker.begin();
}

void soundSetEnabled(bool enabled)
{
    systemSoundEnabled = enabled;
}

bool soundIsEnabled()
{
    return systemSoundEnabled;
}

void soundSetVolume(uint8_t volume)
{
    // Runtime UI volume. Boot chime intentionally ignores this and uses
    // the old fixed volume 90.
    systemVolume = volume;
    M5Cardputer.Speaker.setVolume(systemVolume);
}

static void toneWait(int freq, int duration, int pauseMs = 20)
{
    if (!systemSoundEnabled) return;

    M5Cardputer.Speaker.begin();
    M5Cardputer.Speaker.setVolume(systemVolume);
    M5Cardputer.Speaker.tone(freq, duration);
    delay(duration + pauseMs);
}

void soundBootChime()
{
    if (!systemSoundEnabled) return;

    M5Cardputer.Speaker.setVolume(90);

    // Silent warm-up after cold power-on.
    // No audible wake pulses.
    delay(350);

    // Soft fade-in style boot chime.
    toneWait(660, 55, 45);
    toneWait(880, 90, 55);
    toneWait(990, 180, 0);
    M5Cardputer.Speaker.stop();
}

void soundBoot()
{
    soundBootChime();
}

void soundSuccess()
{
    toneWait(1200, 50, 0);
    M5Cardputer.Speaker.stop();
}

void soundError()
{
    toneWait(300, 120, 0);
    M5Cardputer.Speaker.stop();
}

void soundSave()
{
    toneWait(900, 40, 35);
    toneWait(1300, 60, 0);
    M5Cardputer.Speaker.stop();
}

void soundRun()
{
    toneWait(1000, 45, 0);
    M5Cardputer.Speaker.stop();
}

void soundTest()
{
    if (!systemSoundEnabled) return;
    // Quiet diagnostic, not a startup sound replacement.
    M5Cardputer.Speaker.setVolume(90);
    toneWait(700, 80, 35);
    toneWait(1100, 80, 0);
    M5Cardputer.Speaker.setVolume(systemVolume);
}

void soundSilence()
{
    M5Cardputer.Speaker.stop();
}
