#include "shell_internal.h"

// WELCOME / HELP / INFO
// ───────────────────────────────────────────────────────────────────
void showWelcome() {
    addHistory("idDOS:\\>", GREEN);
    addHistory("");
    addHistory("WELCOME TO THE SYSTEM, " + upperCopy(configUsername()), WHITE);
    addHistory("");
    addHistory("TYPE help TO BEGIN", DARKGREY);
    addHistory("");
}

void showHelpText()
{
    addHistory("");
    addHistory("COMMANDS", CYAN);
    addHistory("");

    addHelpRow("dir",    "show content");
    addHelpRow("cd",     "enter folder");
    addHelpRow("cd..",   "back");

    addHelpRow("new",    "create file");
    addHelpRow("mkdir",  "create folder");
    addHelpRow("del",    "delete file/dir");

    addHelpRow("copy",   "copy file/dir");
    addHelpRow("paste",  "paste here");

    addHelpRow("open",   "open file");
    addHelpRow("cat",    "show file");

    addHelpRow("cls",    "clear");
    addHelpRow("help",   "help");
    addHelpRow("info",   "system");
    addHelpRow("lock",   "lock");
    addHelpRow("reboot", "restart");
    addHelpRow("ver",    "version");
    addHelpRow("about",  "about");

    addHistory("");
    addHistory("HW TOOLS", CYAN);
    addHelpRow("i2c",    "scan G1/G2");
    addHelpRow("spi",    "test EXT");
    addHelpRow("uart",   "console");
    addHelpRow("gpio",   "monitor");
    addHelpRow("pwm",    "generate");
    addHelpRow("dac",    "pwm-dac");
    addHelpRow("logic",  "probe");
    addHelpRow("oled",   "test I2C");
    addHelpRow("display","lcd test");
    addHelpRow("rtc",    "time");
}

void showInfo() {
    addHistory("");
    addHistory("idDOS Cardputer-Adv", CYAN);
    addHistory("ver   " + String(IDDOS_VERSION));
    addHistory("user  " + configUsername());
    addHistory("cpu   " + String(ESP.getCpuFreqMHz()) + "MHz");
    addHistory("heap  " + String(ESP.getFreeHeap() / 1024) + " KB");
    addHistory("flash " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
    addHistory("bat   " + String(readBatPct()) + "%");

    if (networkConnected()) {
        addHistory("wifi  " + networkCurrentSSID(), GREEN);
        addHistory("ip    " + networkIP(), WHITE);
    } else {
        addHistory("wifi  disconnected", DARKGREY);
    }

    uint64_t tot = storageTotalBytes();
    uint64_t use = storageUsedBytes();

    if (tot > 0) {
        addHistory("sd    " + String((int)((use * 100) / tot)) + "% used");
    } else {
        addHistory("sd    N/A");
    }
}

// ───────────────────────────────────────────────────────────────────
// DIRS
// ───────────────────────────────────────────────────────────────────
void dirRow(const String& name, const String& tag) {
    addHistory(padRight(upperCopy(name), 20) + tag, WHITE);
}

void showRootDir() {
    addHistory("");
    addHistory("MAIN MENU", CYAN);
    addHistory("");

    dirRow("options", "<DIR>");
    dirRow("lab", "<DIR>");
    dirRow("sd", "<DIR>");
}

void showOptDir() {
    addHistory("");
    dirRow("username", "<CFG>");
    dirRow("password", "<CFG>");
    dirRow("sound", "<CFG>");
    dirRow("brightness", "<CFG>");
    dirRow("volume", "<CFG>");
}

void showLabDir() {
    addHistory("");
    addHistory("LAB MENU", CYAN);
    addHistory("");

    dirRow("wlan", "<NET>");
    dirRow("ble", "<GATT>");
    dirRow("ball", "<GAME>");
    dirRow("rec", "<AUDIO>");
    dirRow("tools", "<HW>");
}

void showSdDir() {
    addHistory("");
    addHistory("SD FILE MANAGER", CYAN);
    addHistory("");

    File dir = SD.open(S.sdPath);
    if (!dir || !dir.isDirectory()) {
        addHistory(dir ? "NOT DIRECTORY" : "SD OPEN FAIL", RED);
        if (dir) dir.close();
        return;
    }

    int cnt = 0;
    for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
        String n = cleanName(String(e.name()));
        if (n.length() > 0 && !isHiddenEntry(n)) {
            dirRow(n, e.isDirectory() ? "<DIR>" : "<FILE>");
            cnt++;
        }
        e.close();
    }

    dir.close();
    if (cnt == 0) addHistory("EMPTY FOLDER", DARKGREY);
}

// ───────────────────────────────────────────────────────────────────
// SCROLLBAR
// ───────────────────────────────────────────────────────────────────
void drawMenuScrollbar(int total, int visible, int selected, int y, int h) {
    if (total <= visible) return;

    int x = Dim::SCROLL_X;

    M5Cardputer.Display.drawRect(x, y, 3, h, DARKGREY);

    int thumbH = (h * visible) / total;
    if (thumbH < 8) thumbH = 8;
    if (thumbH > h - 2) thumbH = h - 2;

    int maxSel = total - 1;
    int thumbY = y + 1;

    if (maxSel > 0) {
        thumbY = y + 1 + ((h - 2 - thumbH) * selected) / maxSel;
    }

    M5Cardputer.Display.fillRect(x + 1, thumbY, 1, thumbH, GREEN);
}

// ───────────────────────────────────────────────────────────────────
// OPTIONS
// ───────────────────────────────────────────────────────────────────
String optBar(int v) {
    String s = "[";
    for (int i = 0; i < Limit::OPTION_MAX; i++) s += (i < v) ? '|' : ' ';
    s += ']';
    return s;
}

String passText() {
    return configPasswordMasked();
}

void showOptions() {
    S.uiMode = UI_OPTIONS;
    S.section = OPTION;

    if (S.optSel < 0) S.optSel = 0;
    if (S.optSel >= Limit::OPT_TOTAL) S.optSel = Limit::OPT_TOTAL - 1;

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "OPTIONS", CYAN);

    for (int idx = 0; idx < Limit::OPT_TOTAL; idx++) {
        int y = Dim::HIST_Y0 + 18 + idx * 15;
        bool sel = (S.optSel == idx);

        printAt(5, y, sel ? ">" : " ", GREEN);

        if (idx == 0) {
            printAt(18, y, "USERNAME", sel ? GREEN : WHITE);
            printAt(105, y, trimToWidth(configUsername(), 18), CYAN);
        }
        else if (idx == 1) {
            printAt(18, y, "PASSWORD", sel ? GREEN : WHITE);
            printAt(105, y, trimToWidth(passText(), 18), YELLOW);
        }
        else if (idx == 2) {
            printAt(18, y, "SOUND", sel ? GREEN : WHITE);
            printAt(105, y, soundIsEnabled() ? "ON" : "OFF",
                    soundIsEnabled() ? GREEN : DARKGREY);
        }
        else if (idx == 3) {
            printAt(18, y, "BRIGHT", sel ? GREEN : WHITE);
            printAt(105, y, optBar(S.brightness), GREEN);
        }
        else if (idx == 4) {
            printAt(18, y, "VOLUME", sel ? GREEN : WHITE);
            printAt(105, y, optBar(S.volume), GREEN);
        }
    }

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER edit  UP/DOWN select", WHITE);
}

void optUp() {
    if (S.optSel > 0) S.optSel--;
    showOptions();
}

void optDown() {
    if (S.optSel < Limit::OPT_TOTAL - 1) S.optSel++;
    showOptions();
}

void optLeft() {
    if (S.optSel == 3 && S.brightness > 0) {
        S.brightness--;
        applyFullBrightness();
    }
    else if (S.optSel == 4 && S.volume > 0) {
        S.volume--;
        applyVolume();
    }
    else if (S.optSel == 2) {
        soundSetEnabled(!soundIsEnabled());
        soundSuccess();
    }

    showOptions();
}

void optRight() {
    if (S.optSel == 3 && S.brightness < Limit::OPTION_MAX) {
        S.brightness++;
        applyFullBrightness();
    }
    else if (S.optSel == 4 && S.volume < Limit::OPTION_MAX) {
        S.volume++;
        applyVolume();
    }
    else if (S.optSel == 2) {
        soundSetEnabled(!soundIsEnabled());
        soundSuccess();
    }

    showOptions();
}

void startConfigEdit(EditKind kind) {
    S.uiMode = UI_CONFIG_EDIT;
    S.editKind = kind;

    if (kind == EDIT_USERNAME) S.editBuffer = configUsername();
    else if (kind == EDIT_PASSWORD) S.editBuffer = "";
    else S.editBuffer = "";

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, kind == EDIT_USERNAME ? "SET USERNAME" : "SET NEW PASSWORD", CYAN);
    printAt(5, 48, ">", GREEN);
    String shown = S.editBuffer;
    if (kind == EDIT_PASSWORD) {
        shown = "";
        for (int i = 0; i < S.editBuffer.length(); ++i) shown += '*';
    }
    printAt(18, 48, trimToWidth(shown + "_", 34), WHITE);
    printAt(5, Dim::PROMPT_Y, "ENTER SAVE  ESC BACK  BSP DEL", DARKGREY);
}

void renderConfigEdit() {
    M5Cardputer.Display.fillRect(0, 42, Dim::SCREEN_W, 30, BLACK);
    printAt(5, 48, ">", GREEN);
    String shown = S.editBuffer;
    if (S.editKind == EDIT_PASSWORD) {
        shown = "";
        for (int i = 0; i < S.editBuffer.length(); ++i) shown += '*';
    }
    printAt(18, 48, trimToWidth(shown + "_", 34), WHITE);
}

void saveConfigEdit() {
    if (S.editKind == EDIT_USERNAME) {
        configSetUsername(S.editBuffer);
        configSave();
        soundSave();
    }
    else if (S.editKind == EDIT_PASSWORD) {
        configSetPassword(S.editBuffer);
        configSave();
        soundSave();
    }

    S.editKind = EDIT_NONE;
    S.editBuffer = "";
    showOptions();
}

// ───────────────────────────────────────────────────────────────────
// LAB
// ───────────────────────────────────────────────────────────────────
void showLabScreen() {
    S.uiMode = UI_LAB;
    S.section = LAB_SECTION;

    if (S.labSel < 0) S.labSel = 0;
    if (S.labSel >= Limit::LAB_TOTAL) S.labSel = Limit::LAB_TOTAL - 1;
    if (S.labSel < S.labScroll) S.labScroll = S.labSel;
    if (S.labSel >= S.labScroll + Limit::LAB_VISIBLE) S.labScroll = S.labSel - Limit::LAB_VISIBLE + 1;
    if (S.labScroll < 0) S.labScroll = 0;

    static const char* names[] = { "WLAN", "BLE", "BALL", "REC", "TOOLS" };
    static const char* tags[]  = { "<NET>", "<RADIO>", "<IMU>", "<AUDIO>", "<HW>" };

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "LABORATORY", CYAN);

    int y = 36;
    for (int row = 0; row < Limit::LAB_VISIBLE; ++row) {
        int idx = S.labScroll + row;
        if (idx >= Limit::LAB_TOTAL) break;
        bool sel = idx == S.labSel;
        printAt(5, y, sel ? ">" : " ", GREEN);
        printAt(18, y, names[idx], sel ? GREEN : WHITE);
        printAt(105, y, tags[idx], DARKGREY);
        y += 17;
    }

    drawMenuScrollbar(Limit::LAB_TOTAL, Limit::LAB_VISIBLE, S.labSel, 36, 68);

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER open  UP/DOWN move", WHITE);
}

// ───────────────────────────────────────────────────────────────────
