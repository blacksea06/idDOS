#include <M5Cardputer.h>
#include "keys_compat.h"
#include "boot.h"
#include "storage.h"
#include "shell.h"
#include "sound.h"
#include "config.h"
#include "network.h"
#include "bluetooth_lab.h"
#include "system_log.h"

enum AppMode
{
    MODE_BOOT,
    MODE_UNLOCK,
    MODE_SHELL
};

static AppMode mode = MODE_BOOT;

static bool sdOk = false;
static bool layoutOk = false;

static String unlockInput = "";
static bool unlockDirty = true;
static uint8_t unlockFailures = 0;
static uint32_t unlockLockedUntilMs = 0;

static void drawUnlock()
{
    auto &d = M5Cardputer.Display;

    d.fillScreen(BLACK);
    d.setTextSize(1);

    d.setTextColor(CYAN, BLACK);
    d.setCursor(5, 8);
    d.print("idDOS UNLOCK");

    d.setTextColor(WHITE, BLACK);
    d.setCursor(5, 32);
    d.print("USER: ");
    d.print(configUsername());

    d.setCursor(5, 56);
    d.print("PASSWORD:");

    d.setTextColor(GREEN, BLACK);
    d.setCursor(5, 72);

    for (int i = 0; i < unlockInput.length(); i++)
    {
        d.print('*');
    }

    d.print('_');

    d.setTextColor(DARKGREY, BLACK);
    d.setCursor(5, 118);
    d.print("ENTER OK  BSP DEL");
}

static void unlockUpdate()
{
    if (unlockDirty)
    {
        drawUnlock();
        unlockDirty = false;
    }

    if (millis() < unlockLockedUntilMs)
    {
        return;
    }

    if (!M5Cardputer.Keyboard.isChange() ||
        !M5Cardputer.Keyboard.isPressed())
    {
        return;
    }

    auto s = M5Cardputer.Keyboard.keysState();

    if (keyBackspace(s))
    {
        if (unlockInput.length() > 0)
        {
            unlockInput.remove(unlockInput.length() - 1);
            unlockDirty = true;
        }
        return;
    }

    if (s.enter)
    {
        if (configCheckPassword(unlockInput))
        {
            unlockFailures = 0;
            unlockLockedUntilMs = 0;
            mode = MODE_SHELL;
            unlockInput = "";
            shellStart();
        }
        else
        {
            unlockFailures++;
            uint32_t penaltyMs = (unlockFailures >= 5) ? 5000UL : 750UL;
            unlockLockedUntilMs = millis() + penaltyMs;
            unlockInput = "";
            unlockDirty = true;
            soundError();
        }
        return;
    }

    for (auto c : s.word)
    {
        if (c >= 32 && c <= 126 && unlockInput.length() < 64)
        {
            unlockInput += c;
            unlockDirty = true;
        }
    }
}

static void enterSystem()
{
    if (configHasPassword())
    {
        mode = MODE_UNLOCK;
        unlockInput = "";
        unlockDirty = true;
        unlockLockedUntilMs = 0;
        return;
    }

    mode = MODE_SHELL;
    shellStart();
}

void setup()
{
    auto cfg = M5.config();

    M5Cardputer.begin(cfg, true);

    Serial.begin(115200);
    delay(300);

    soundBegin();
    networkBegin();
    // bleBegin();
    sdOk = storageInit();
    layoutOk = sdOk && storageCheckLayout();
    if (sdOk && !layoutOk)
    {
        storageCreateLayout();
        layoutOk = storageCheckLayout();
    }

    if (sdOk)
    {
        configLoad();
        systemLog("BOOT");
    }

    bootScreen(sdOk, layoutOk);

    delay(700);
    soundBootChime();
}

void loop()
{
    M5Cardputer.update();

    if (mode == MODE_BOOT)
    {
        auto s = M5Cardputer.Keyboard.keysState();

        if (M5Cardputer.Keyboard.isChange() &&
            M5Cardputer.Keyboard.isPressed() &&
            s.enter)
        {
            enterSystem();
        }

        return;
    }

    if (mode == MODE_UNLOCK)
    {
        unlockUpdate();
        return;
    }

    shellUpdate();

    if (shellLockRequested())
    {
        shellClearLockRequest();
        if (configHasPassword())
        {
            mode = MODE_UNLOCK;
            unlockInput = "";
            unlockDirty = true;
            unlockLockedUntilMs = 0;
        }
    }
}