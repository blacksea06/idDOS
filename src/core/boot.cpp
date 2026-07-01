#include <M5Cardputer.h>
#include "boot.h"
#include "version.h"

static void printStatus(int y, const char* label, bool ok)
{
    auto &d = M5Cardputer.Display;

    d.setTextColor(WHITE, BLACK);
    d.setCursor(5, y);
    d.print(label);

    d.setTextColor(ok ? GREEN : RED, BLACK);
    d.setCursor(70, y);
    d.print(ok ? "OK" : "FAIL");
}

void bootScreen(bool sdOk, bool layoutOk)
{
    auto &d = M5Cardputer.Display;

    d.fillScreen(BLACK);

    d.setTextColor(GREEN, BLACK);
    d.setTextSize(2);
    d.setCursor(5, 5);
    d.print("idDOS");

    // Keep version locked to the right side without pushing the idDOS title.
    d.setCursor(174, 5);
    d.print("V");
    d.print(IDDOS_VERSION);

    d.setTextSize(1);
    d.setTextColor(CYAN, BLACK);
    d.setCursor(5, 25);
    d.print("direct human-machine shell");

    d.setTextColor(WHITE, BLACK);

    d.setCursor(5, 45);
    d.printf("CPU     ESP32-S3 %uMHz", ESP.getCpuFreqMHz());

    d.setCursor(5, 57);
    d.printf("HEAP    %uKB", ESP.getFreeHeap() / 1024);

    d.setCursor(5, 69);
    d.printf("FLASH   %uMB", ESP.getFlashChipSize() / 1024 / 1024);

    d.setCursor(5, 81);
    d.printf("LCD     %dx%d", d.width(), d.height());

    printStatus(96, "SD", sdOk);
    printStatus(108, "LAYOUT", layoutOk);

    d.setCursor(5, 122);

    if (sdOk && layoutOk)
    {
        d.setTextColor(GREEN, BLACK);
        d.print("READY");
    }
    else if (sdOk && !layoutOk)
    {
        d.setTextColor(YELLOW, BLACK);
        d.print("READY LIMITED: RUN INIT");
    }
    else
    {
        d.setTextColor(RED, BLACK);
        d.print("READY LIMITED: SD FAIL");
    }
}
