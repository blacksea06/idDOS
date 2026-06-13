#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "storage.h"

static const int SD_CS   = 12;
static const int SD_MOSI = 14;
static const int SD_SCK  = 40;
static const int SD_MISO = 39;

static SPIClass sdSPI(FSPI);

bool storageInit()
{
    Serial.println("SD INIT...");

    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS, sdSPI))
    {
        Serial.println("SD INIT FAIL");
        return false;
    }

    Serial.println("SD INIT OK");

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("SD size: ");
    Serial.print(cardSize);
    Serial.println(" MB");

    return true;
}

bool storageCheckLayout()
{
    if (!SD.exists("/scripts"))
    {
        Serial.println("Missing /scripts");
        return false;
    }

    if (!SD.exists("/data"))
    {
        Serial.println("Missing /data");
        return false;
    }

    Serial.println("SD layout OK");
    return true;
}