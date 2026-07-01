#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "storage.h"

static const int SD_CS   = 12;
static const int SD_MOSI = 14;
static const int SD_SCK  = 40;
static const int SD_MISO = 39;

static SPIClass sdSPI(FSPI);
static bool sdReady = false;

static const char* REQUIRED_DIRS[] = {
    "/scripts",
    "/apps",
    "/data",
    "/data/tmp",
    "/data/idchat",
    "/audio",
    "/backup",
    "/docs"
};

static bool mkdirIfMissing(const char* path)
{
    if (SD.exists(path)) return true;
    return SD.mkdir(path);
}

bool storageInit()
{
    Serial.println("SD INIT...");

    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    sdReady = SD.begin(SD_CS, sdSPI);

    if (!sdReady)
    {
        Serial.println("SD INIT FAIL");
        return false;
    }

    Serial.println("SD INIT OK");

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("SD SIZE: ");
    Serial.print(cardSize);
    Serial.println(" MB");

    return true;
}

bool storageReady()
{
    return sdReady;
}

bool storageCheckLayout()
{
    if (!sdReady)
    {
        Serial.println("SD NOT READY");
        return false;
    }

    bool ok = true;
    for (const char* path : REQUIRED_DIRS)
    {
        if (!SD.exists(path))
        {
            Serial.print("LAYOUT MISSING ");
            Serial.println(path);
            ok = false;
        }
    }

    if (ok) Serial.println("SD LAYOUT OK");
    return ok;
}

bool storageCreateLayout()
{
    if (!sdReady) return false;

    bool ok = true;
    for (const char* path : REQUIRED_DIRS)
    {
        ok &= mkdirIfMissing(path);
    }

    return ok;
}

uint64_t storageTotalBytes()
{
    if (!sdReady) return 0;
    return SD.totalBytes();
}

uint64_t storageUsedBytes()
{
    if (!sdReady) return 0;
    return SD.usedBytes();
}

const char* storageAudioDir()
{
    return "/audio";
}
