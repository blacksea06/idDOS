#include "shell_internal.h"

// COMMANDS
// ───────────────────────────────────────────────────────────────────

void showVersion() {
    addHistory("");
    addHistory("idDOS " + String(IDDOS_VERSION), CYAN);
    addHistory(String("build ") + IDDOS_BUILD, WHITE);
    addHistory("target Cardputer-Adv", WHITE);
}

void showAbout() {
    addHistory("");
    addHistory("ABOUT idDOS", CYAN);
    addHistory("command OS for hardware", WHITE);
    addHistory("files commands lab audio", WHITE);
    addHistory("id = desire", DARKGREY);
    addHistory("DOS = command path", DARKGREY);
    addHistory("desire becomes command", GREEN);
}

void executeCommand(const String& raw) {
    String cmd = raw;
    cmd.trim();

    String lower = lowerCopy(cmd);

    // While the welcome text is visible, a plain OK/ENTER should only
    // confirm that the system is alive. It must not clear the greeting
    // or stack empty id:\> prompts.
    if (S.welcomeActive && lower == "") {
        S.inputLine = "";
        S.cmdOffset = 0;
        drawBottomBar();
        return;
    }

    if (S.welcomeActive) {
        S.history.clear();
        S.scrollOffset = 0;
        S.welcomeActive = false;
        addBreadcrumb();
    }

    addHistory(prompt() + S.inputLine, GREEN);

    if (lower == "") {
        // ok
    }
    else if (lower == "dir") {
        if (S.section == ROOT) showRootDir();
        else if (S.section == OPTION) showOptDir();
        else if (S.section == LAB_SECTION) showLabDir();
        else if (S.section == SD_SECTION) showSdDir();
    }
    else if (lower == "help") {
        showHelpText();
    }
    else if (lower == "info") {
        showInfo();
    }
    else if (lower == "ver") {
        showVersion();
    }
    else if (lower == "about") {
        showAbout();
    }
    else if (lower == "beep" || lower == "soundtest" || lower == "sound test") {
        addHistory("SOUND TEST", CYAN);
        addHistory("listen for 2 tones", DARKGREY);
        renderAll();
        soundTest();
    }
    else if (lower == "lock") {
        lockRequested = true;
        addHistory("LOCK", YELLOW);
        systemLog("LOCK");
    }
    else if (lower == "reboot" || lower == "restart") {
        addHistory("REBOOT...", YELLOW);
        systemLog("REBOOT");
        renderAll();
        delay(250);
        ESP.restart();
    }
    else if (lower == "cls" || lower == "clean" || lower == "clian") {
        clearShell();
        return;
    }
    else if (lower == "init") {
        initLayout();
    }
    else if (lower == "cd .." || lower == "cd..") {
        if (S.section == SD_SECTION) sdGoBack();
        else S.section = ROOT;

        S.uiMode = UI_SHELL;
        addBreadcrumb();
    }
    else if (lower == "cd option" || lower == "cd options") {
        if (S.section == ROOT) {
            S.section = OPTION;
            S.inputLine = "";
            S.cmdOffset = 0;
            S.optSel = 0;
            S.optScroll = 0;
            showOptions();
            return;
        } else {
            addHistory("BAD PATH", RED);
            soundError();
        }
    }
    else if (lower == "cd lab") {
        if (S.section == ROOT) {
            S.section = LAB_SECTION;
            S.inputLine = "";
            S.cmdOffset = 0;
            S.labSel = 0;
            S.labScroll = 0;
            showLabScreen();
            return;
        } else {
            addHistory("BAD PATH", RED);
            soundError();
        }
    }
    else if (lower == "cd sd") {
        if (S.section == ROOT) {
            S.section = SD_SECTION;
            S.sdPath = "/";
            addBreadcrumb();
        } else {
            addHistory("BAD PATH", RED);
            soundError();
        }
    }
    else if (lower == "cd wlan" && S.section == LAB_SECTION) {
        bleStopForWifi();
        delay(250);
        showWlanList(true);
        return;
    }
    else if (lower == "scan" && S.section == LAB_SECTION) {
        bleStopForWifi();
        delay(250);
        showWlanList(true);
        return;
    }
    else if (lower == "cd ble" && S.section == LAB_SECTION) {
        showBleScreen();
        return;
    }
    else if (lower == "cd ball" && S.section == LAB_SECTION) {
        startBallApp();
        return;
    }
    else if ((lower == "cd rec" || lower == "cd audio") && S.section == LAB_SECTION) {
        showAudioRecorderScreen();
        return;
    }
    else if ((lower == "cd tools" && S.section == LAB_SECTION) || lower == "tools") {
        HardwareTools::openMenu();
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if ((lower == "i2c" || lower == "i2c scan") ) {
        HardwareTools::openTool(HardwareTools::TOOL_I2C);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if ((lower == "spi" || lower == "spi test") ) {
        HardwareTools::openTool(HardwareTools::TOOL_SPI);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "uart" ) {
        HardwareTools::openTool(HardwareTools::TOOL_UART);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "gpio" ) {
        HardwareTools::openTool(HardwareTools::TOOL_GPIO);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "pwm" ) {
        HardwareTools::openTool(HardwareTools::TOOL_PWM);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "dac" ) {
        HardwareTools::openTool(HardwareTools::TOOL_DAC);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "logic" ) {
        HardwareTools::openTool(HardwareTools::TOOL_LOGIC);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "oled" ) {
        HardwareTools::openTool(HardwareTools::TOOL_OLED);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "display" ) {
        HardwareTools::openTool(HardwareTools::TOOL_DISPLAY);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower == "rtc" ) {
        HardwareTools::openTool(HardwareTools::TOOL_RTC);
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    else if (lower.startsWith("cd ") && S.section == SD_SECTION) {
        String name = cmd.substring(3);
        name.trim();

        String next;
        if (!buildSdPath(name, next)) {
            addHistory("INVALID PATH", RED);
            soundError();
        } else {
            File d = SD.open(next);
            if (d && d.isDirectory()) {
                S.sdPath = next;
                addBreadcrumb();
            } else {
                addHistory("BAD PATH", RED);
                soundError();
            }
            if (d) d.close();
        }
    }
    else if (lower.startsWith("new ")) {
        newFile(cmd.substring(4));
    }
    else if (lower.startsWith("mkdir ")) {
        mkDir(cmd.substring(6));
    }
    else if (lower.startsWith("del ")) {
        delEntry(cmd.substring(4));
    }
    else if (lower.startsWith("rmdir ")) {
        delEntry(cmd.substring(6)); // hidden compatibility alias
    }
    else if (lower.startsWith("copy ")) {
        copyEntry(cmd.substring(5));
    }
    else if (lower == "paste") {
        pasteEntry();
    }
    else if (lower.startsWith("cat ")) {
        catFile(cmd.substring(4));
    }
    else if (lower.startsWith("open ")) {
        openTextEditor(cmd.substring(5));
        if (editorMode || S.uiMode == UI_AUDIO_PLAYER) {
            S.inputLine = "";
            autoCmdEnd();
            return;
        }
    }
    else {
        addHistory("UNKNOWN COMMAND", RED);
        soundError();
    }

    S.inputLine = "";
    autoCmdEnd();
    renderAll();
}

// ───────────────────────────────────────────────────────────────────
// PUBLIC API
// ───────────────────────────────────────────────────────────────────

bool shellLockRequested() {
    return lockRequested;
}

void shellClearLockRequest() {
    lockRequested = false;
}

void shellStart() {
    S = ShellState{};

    S.history.reserve(Limit::MAX_HISTORY);
    S.inputLine.reserve(Limit::INPUT_MAX);
    S.editBuffer.reserve(64);
    S.wlanPassword.reserve(Limit::PASSWORD_MAX);
    S.audioNameEdit.reserve(32);
    S.clipPath.reserve(80);
    S.clipName.reserve(40);
    editorLines.reserve(Limit::EDITOR_MAX_LINES);

    applyFullBrightness();
    applyVolume(false);
    soundBegin();
    HardwareTools::begin();

    showWelcome();

    M5Cardputer.Display.fillScreen(BLACK);
    renderAll();
}
// ───────────────────────────────────────────────────────────────────
// ───────────────────────────────────────────────────────────────────
// BLE GATT SERVER
// ───────────────────────────────────────────────────────────────────
