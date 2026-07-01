#include <Arduino.h>
#include <SD.h>
#include "storage.h"
#include "system_log.h"

#ifndef FILE_APPEND
#define FILE_APPEND "a"
#endif

static const char* LOG_PATH = "/data/system.log";
static const uint32_t LOG_MAX_BYTES = 64UL * 1024UL;

static String cleanEvent(String e)
{
    e.replace("\r", " ");
    e.replace("\n", " ");
    e.trim();
    if (e.length() > 96) e = e.substring(0, 96);
    return e;
}

static void rotateIfNeeded()
{
    if (!SD.exists(LOG_PATH)) return;
    File f = SD.open(LOG_PATH, FILE_READ);
    if (!f) return;
    size_t sz = f.size();
    f.close();

    if (sz <= LOG_MAX_BYTES) return;

    SD.remove("/data/system.old");
    SD.rename(LOG_PATH, "/data/system.old");
}

void systemLog(const String& event)
{
    if (!storageReady()) return;
    if (!SD.exists("/data")) SD.mkdir("/data");

    rotateIfNeeded();

    File f = SD.open(LOG_PATH, FILE_APPEND);
    if (!f) return;
    f.print('[');
    f.print(millis());
    f.print("] ");
    f.println(cleanEvent(event));
    f.close();
}

void systemLogClear()
{
    if (!storageReady()) return;
    if (SD.exists(LOG_PATH)) SD.remove(LOG_PATH);
}
