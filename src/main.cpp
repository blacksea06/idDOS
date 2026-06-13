#include <M5Cardputer.h>
#include "boot.h"
#include "storage.h"
#include "shell.h"

bool shellActive = false;

void setup()
{
    auto cfg = M5.config();

    // true включает клавиатуру Cardputer
    M5Cardputer.begin(cfg, true);

    Serial.begin(115200);
    delay(500);

    bool sdOk = storageInit();
    bool layoutOk = sdOk && storageCheckLayout();

    bootScreen(sdOk && layoutOk);
}

void loop()
{
    M5Cardputer.update();

    if (!shellActive)
    {
        auto s = M5Cardputer.Keyboard.keysState();

        if (M5Cardputer.Keyboard.isChange() &&
            M5Cardputer.Keyboard.isPressed() &&
            s.enter)
        {
            shellActive = true;
            shellStart();
        }

        return;
    }

    shellUpdate();
}