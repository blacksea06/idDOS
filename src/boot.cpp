#include <M5Cardputer.h>
#include "boot.h"

void bootScreen(bool sdOk)
{
    M5Cardputer.Display.fillScreen(BLACK);

    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setTextSize(2);

    M5Cardputer.Display.setCursor(5, 5);
    M5Cardputer.Display.print("idDOS");

    M5Cardputer.Display.setCursor(180, 5);
    M5Cardputer.Display.print("V0.6");

    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.setTextSize(1);

    M5Cardputer.Display.setCursor(5, 25);
    M5Cardputer.Display.print("System File Manager");

    M5Cardputer.Display.setTextColor(WHITE);

    M5Cardputer.Display.setCursor(5, 50);
    M5Cardputer.Display.print("CPU   ESP32-S3 @240MHz");

    M5Cardputer.Display.setCursor(5, 62);
    M5Cardputer.Display.print("RAM   512KB SRAM");

    M5Cardputer.Display.setCursor(5, 74);
    M5Cardputer.Display.print("FLASH 8MB");

    M5Cardputer.Display.setCursor(5, 86);
    M5Cardputer.Display.print("LCD   240x135");

    M5Cardputer.Display.setCursor(5, 108);
    M5Cardputer.Display.print("SD    ");

    if (sdOk)
    {
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.print("OK");
    }
    else
    {
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.print("FAIL");
    }

    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.setCursor(5, 122);
    M5Cardputer.Display.print("READY");
}