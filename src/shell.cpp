#include <M5Cardputer.h>
#include <SD.h>
#include <vector>
#include "shell.h"

enum Section { ROOT, OPTION, SD_SECTION };

struct TermLine {
    String text;
    uint16_t color;
};

static Section currentSection = ROOT;
static String inputLine = "";
static String sdPath = "/";

static bool helpMode = false;
static bool optionsMode = false;
static bool editorMode = false;
static bool editorSaveMode = false;

static int helpScroll = 0;

static int optionSelected = 0;
static int brightnessLevel = 4;
static int volumeLevel = 3;

static std::vector<TermLine> history;
static std::vector<String> editorLines;

static String editorPath = "";
static String editorName = "";

static int scrollOffset = 0;
static int commandOffset = 0;
static int editorScrollY = 0;
static int editorScrollX = 0;

static int batteryPercent = 0;
static uint32_t lastBatteryUpdate = 0;

static const int SCREEN_W = 240;
static const int SCREEN_H = 135;
static const int TEXT_SIZE = 1;
static const int CHAR_W = 7;
static const int LINE_H = 13;

static const int X0 = 5;
static const int TOP_Y = 4;
static const int BATTERY_X = 154;
static const int BATTERY_W = 86;

static const int HISTORY_Y0 = 5;
static const int SEP_Y = 112;
static const int PROMPT_Y = 123;

static const int MAX_HISTORY_LINES = 250;
static const int OPTION_MAX = 6;
static const int EDITOR_MAX_LINES = 300;
static const int EDITOR_MAX_COLS = 100;

static const uint32_t BATTERY_UPDATE_MS = 30000;

static const char* helpCmds[] =
{
    "dir     show folder",
    "cd      open folder",
    "cd..    go back",
    "help    all commands",
    "info    system data",
    "clean   clear screen",
    "run     start MAIN.ID",
    "new     create txt",
    "open    edit txt",
    "del     delete txt",
    "fn up   scroll up",
    "fn down scroll down",
    "fn left scroll left",
    "fn right scroll right",
    "esc     return"
};

static const int HELP_COUNT = sizeof(helpCmds) / sizeof(helpCmds[0]);

static String upperCopy(String s) {
    s.toUpperCase();
    return s;
}

static String cleanName(String name) {
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    return name;
}

static bool isHiddenSystemEntry(String name) {
    String n = name;
    n.toLowerCase();
    return n == "system volume information" ||
           n == ".trashes" ||
           n == ".spotlight-v100" ||
           n == ".fseventsd";
}

static String prompt() {
    if (currentSection == ROOT) return "id:\\>";
    if (currentSection == OPTION) return "option:\\>";
    if (sdPath == "/") return "sd:\\>";

    String p = sdPath;
    p.replace("/", "\\");
    return "sd:" + p + "\\>";
}

static String currentPath() {
    if (currentSection == ROOT) return "id:\\>";
    if (currentSection == OPTION) return "id:\\>OPTIONS\\";

    String p = "id:\\>SD";
    if (sdPath != "/") {
        String s = sdPath;
        s.replace("/", "\\");
        p += s;
    }
    p += "\\";
    return p;
}

static void setText(uint16_t color) {
    M5Cardputer.Display.setTextSize(TEXT_SIZE);
    M5Cardputer.Display.setTextColor(color, BLACK);
}

static void printAt(int x, int y, const String& text, uint16_t color = WHITE) {
    setText(color);
    M5Cardputer.Display.setCursor(x, y);
    M5Cardputer.Display.print(text);
}

static int readBatteryPercent() {
    int level = M5Cardputer.Power.getBatteryLevel();
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return level;
}

static String batteryBars(int percent) {
    int bars = 0;
    if (percent > 80) bars = 5;
    else if (percent > 60) bars = 4;
    else if (percent > 40) bars = 3;
    else if (percent > 20) bars = 2;
    else if (percent > 5) bars = 1;

    String out = "[";
    for (int i = 0; i < 5; i++) out += (i < bars) ? "|" : " ";
    out += "] ";
    out += String(percent);
    out += "%";
    return out;
}

static void addHistory(const String& text = "", uint16_t color = WHITE) {
    if (history.size() >= MAX_HISTORY_LINES) history.erase(history.begin());
    history.push_back({text, color});
    scrollOffset = 0;
}

static void addPathLine() {
    addHistory(currentPath(), GREEN);
}

static bool updateBattery(bool force = false) {
    if (!force && millis() - lastBatteryUpdate < BATTERY_UPDATE_MS) return false;
    lastBatteryUpdate = millis();

    int newPercent = readBatteryPercent();
    bool changed = force || newPercent != batteryPercent;
    batteryPercent = newPercent;
    return changed;
}

static void drawBattery() {
    M5Cardputer.Display.fillRect(BATTERY_X, TOP_Y, BATTERY_W, LINE_H, BLACK);
    printAt(BATTERY_X, TOP_Y, batteryBars(batteryPercent), GREEN);
}

static int visibleHistoryLines() {
    return (SEP_Y - HISTORY_Y0) / LINE_H;
}

static void renderHistory() {
    M5Cardputer.Display.fillRect(0, 0, SCREEN_W, SEP_Y, BLACK);
    drawBattery();

    int visible = visibleHistoryLines();
    int total = history.size();
    int maxScroll = total > visible ? total - visible : 0;

    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    int start = total - visible - scrollOffset;
    if (start < 0) start = 0;

    int y = HISTORY_Y0;
    for (int i = start; i < total && y < SEP_Y - LINE_H + 1; i++) {
        printAt(X0, y, history[i].text, history[i].color);
        y += LINE_H;
    }
}

static String commandLine() {
    return prompt() + inputLine + "_";
}

static int maxCommandChars() {
    return (SCREEN_W - X0) / CHAR_W;
}

static void fixCommandOffset() {
    int maxChars = maxCommandChars();
    int total = commandLine().length();

    if (commandOffset < 0) commandOffset = 0;

    if (total <= maxChars) {
        commandOffset = 0;
        return;
    }

    int maxOffset = total - maxChars;
    if (commandOffset > maxOffset) commandOffset = maxOffset;
}

static void autoScrollCommandEnd() {
    int maxChars = maxCommandChars();
    int total = commandLine().length();
    commandOffset = total > maxChars ? total - maxChars : 0;
}

static void drawCommandLine() {
    fixCommandOffset();

    String line = commandLine();
    int maxChars = maxCommandChars();

    int end = commandOffset + maxChars;
    if (end > line.length()) end = line.length();

    int promptLen = prompt().length();
    int inputEnd = promptLen + inputLine.length();

    int x = X0;

    for (int i = commandOffset; i < end; i++) {
        String ch = String(line[i]);
        if (i < promptLen || i == inputEnd) printAt(x, PROMPT_Y, ch, GREEN);
        else printAt(x, PROMPT_Y, ch, WHITE);
        x += CHAR_W;
    }
}

static void drawBottomBar() {
    M5Cardputer.Display.fillRect(0, SEP_Y, SCREEN_W, SCREEN_H - SEP_Y, BLACK);
    printAt(0, SEP_Y, "----------------------------------------", DARKGREY);
    drawCommandLine();
}

static void renderAll() {
    renderHistory();
    drawBottomBar();
}

static void clearShell() {
    history.clear();
    scrollOffset = 0;
    commandOffset = 0;
    inputLine = "";

    addPathLine();

    M5Cardputer.Display.fillScreen(BLACK);
    renderAll();
}

static String padRight(String s, int width) {
    while (s.length() < width) s += " ";
    return s;
}

static void addDirRow(const String& name, const String& type) {
    addHistory(padRight(upperCopy(name), 19) + type, WHITE);
}

static void showRootDir() {
    addHistory("");
    addDirRow("options", "<DIR>");
    addDirRow("sd", "<DIR>");
}

static void showOptionDir() {
    addHistory("");
    addDirRow("brightness", "<CFG>");
    addDirRow("volume", "<CFG>");
}

static void showSdDir() {
    addHistory("");
    addHistory("SYSTEM FILE MANAGER", CYAN);
    addHistory("");

    File dir = SD.open(sdPath);

    if (!dir) {
        addHistory("SD OPEN FAIL", RED);
        return;
    }

    if (!dir.isDirectory()) {
        addHistory("NOT DIRECTORY", RED);
        dir.close();
        return;
    }

    int count = 0;
    File entry = dir.openNextFile();

    while (entry) {
        String name = cleanName(String(entry.name()));

        if (name.length() > 0 && !isHiddenSystemEntry(name)) {
            addDirRow(name, entry.isDirectory() ? "<DIR>" : "<FILE>");
            count++;
        }

        entry.close();
        entry = dir.openNextFile();
    }

    dir.close();

    if (count == 0) addHistory("EMPTY FOLDER", DARKGREY);
}

static void showHelpScreen() {
    helpMode = true;

    M5Cardputer.Display.fillScreen(BLACK);

    printAt(5, 5, "idDOS HELP", CYAN);

    int y = 20;
    int visible = 7;

    if (helpScroll < 0) helpScroll = 0;

    int maxScroll = HELP_COUNT > visible ? HELP_COUNT - visible : 0;
    if (helpScroll > maxScroll) helpScroll = maxScroll;

    for (int i = 0; i < visible; i++) {
        int idx = helpScroll + i;
        if (idx >= HELP_COUNT) break;

        String row = String(helpCmds[idx]);
        int space = row.indexOf(' ');

        if (space > 0) {
            printAt(5, y, row.substring(0, space), GREEN);
            printAt(65, y, row.substring(space + 1), WHITE);
        } else {
            printAt(5, y, row, WHITE);
        }

        y += 12;
    }

    printAt(0, SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, PROMPT_Y, "fn up/down scroll", WHITE);
    printAt(160, PROMPT_Y, "esc", YELLOW);
}

static void showInfo() {
    updateBattery(true);

    addHistory("");
    addHistory("idDOS v0.6", CYAN);
    addHistory("dev   Igor Dubrovsky", WHITE);
    addHistory("");

    addHistory("CPU   " + String(ESP.getCpuFreqMHz()) + "MHz");
    addHistory("RAM   " + String(ESP.getFreeHeap() / 1024) + "KB");
    addHistory("FLASH " + String(ESP.getFlashChipSize() / 1024 / 1024) + "MB");
    addHistory("BAT   " + String(batteryPercent) + "%");

    uint64_t sdTotal = SD.totalBytes();
    uint64_t sdUsed = SD.usedBytes();

    if (sdTotal > 0) {
        int sdPct = (int)((sdUsed * 100) / sdTotal);
        addHistory("SD    " + String(sdPct) + "% USED");
        addHistory("SD U  " + String(sdUsed / 1024 / 1024) + "MB");
        addHistory("SD T  " + String(sdTotal / 1024 / 1024) + "MB");
    } else {
        addHistory("SD    N/A");
    }
}

static int levelToBrightness(int level) {
    return map(level, 0, OPTION_MAX, 20, 255);
}

static int levelToVolume(int level) {
    return map(level, 0, OPTION_MAX, 0, 255);
}

static void applyBrightness() {
    M5Cardputer.Display.setBrightness(levelToBrightness(brightnessLevel));
}

static void applyVolume(bool beep = true) {
    int volume = levelToVolume(volumeLevel);
    M5Cardputer.Speaker.setVolume(volume);

    if (beep && volumeLevel > 0) {
        M5Cardputer.Speaker.tone(500 + volumeLevel * 120, 80);
    }
}

static String optionBar(int level) {
    String s = "[";
    for (int i = 0; i < OPTION_MAX; i++) s += (i < level) ? "|" : " ";
    s += "]";
    return s;
}

static void drawOptionRow(int y, int index, const String& name, int value) {
    bool selected = optionSelected == index;

    printAt(5, y, selected ? ">" : " ", GREEN);
    printAt(20, y, name, selected ? GREEN : WHITE);
    printAt(120, y, optionBar(value), GREEN);
    printAt(190, y, String(value) + "/" + String(OPTION_MAX), selected ? GREEN : WHITE);
}

static void showOptionsScreen() {
    optionsMode = true;

    M5Cardputer.Display.fillScreen(BLACK);

    printAt(5, 5, "id:\\>OPTIONS\\", GREEN);

    drawOptionRow(35, 0, "BRIGHTNESS", brightnessLevel);
    drawOptionRow(55, 1, "VOLUME", volumeLevel);

    printAt(0, SEP_Y, "----------------------------------------", DARKGREY);

    printAt(5, PROMPT_Y, "^v", WHITE);
    printAt(35, PROMPT_Y, "select", WHITE);

    printAt(95, PROMPT_Y, "<>", WHITE);
    printAt(125, PROMPT_Y, "adjust", WHITE);

    printAt(185, PROMPT_Y, "esc", YELLOW);
    printAt(215, PROMPT_Y, "back", WHITE);
}

static void optionsMoveUp() {
    if (optionSelected > 0) optionSelected--;
    showOptionsScreen();
}

static void optionsMoveDown() {
    if (optionSelected < 1) optionSelected++;
    showOptionsScreen();
}

static void optionsAdjustLeft() {
    if (optionSelected == 0 && brightnessLevel > 0) {
        brightnessLevel--;
        applyBrightness();
    }

    if (optionSelected == 1 && volumeLevel > 0) {
        volumeLevel--;
        applyVolume();
    }

    showOptionsScreen();
}

static void optionsAdjustRight() {
    if (optionSelected == 0 && brightnessLevel < OPTION_MAX) {
        brightnessLevel++;
        applyBrightness();
    }

    if (optionSelected == 1 && volumeLevel < OPTION_MAX) {
        volumeLevel++;
        applyVolume();
    }

    showOptionsScreen();
}

static void exitOptions() {
    optionsMode = false;
    renderAll();
}

static String buildSdPath(const String& name) {
    if (sdPath == "/") return "/" + name;
    return sdPath + "/" + name;
}

static void sdGoBack() {
    if (sdPath == "/") {
        currentSection = ROOT;
        return;
    }

    int lastSlash = sdPath.lastIndexOf('/');

    if (lastSlash <= 0) sdPath = "/";
    else sdPath = sdPath.substring(0, lastSlash);
}

static String extractQuotedText(String line) {
    int first = line.indexOf('"');
    int last = line.lastIndexOf('"');

    if (first >= 0 && last > first) return line.substring(first + 1, last);
    return "";
}

static void executeScriptLine(String line, bool& running) {
    line.trim();

    if (line.length() == 0) return;
    if (line.startsWith("#")) return;

    String upper = line;
    upper.toUpperCase();

    if (upper.startsWith("PRINT")) {
        addHistory(extractQuotedText(line), WHITE);
        renderAll();
        return;
    }

    if (upper == "CLS") {
        clearShell();
        return;
    }

    if (upper.startsWith("WAIT")) {
        String v = line.substring(4);
        v.trim();

        int ms = v.toInt();
        if (ms < 0) ms = 0;
        if (ms > 10000) ms = 10000;

        delay(ms);
        return;
    }

    if (upper.startsWith("BEEP")) {
        M5Cardputer.Speaker.tone(900, 100);
        delay(120);
        return;
    }

    if (upper == "END") {
        running = false;
        addHistory("SCRIPT END", GREEN);
        renderAll();
        return;
    }

    addHistory("SCRIPT ERR: " + line, RED);
    renderAll();
}

static void runScript(String arg) {
    if (currentSection != SD_SECTION) {
        addHistory("run only in sd", RED);
        return;
    }

    arg.trim();

    String fileName = arg;
    if (fileName.length() == 0) fileName = "MAIN.ID";

    String scriptPath = buildSdPath(fileName);

    File script = SD.open(scriptPath);

    if (!script) {
        addHistory("MAIN.ID NOT FOUND", RED);
        return;
    }

    if (script.isDirectory()) {
        addHistory("NOT A SCRIPT", RED);
        script.close();
        return;
    }

    addHistory("RUNNING " + upperCopy(fileName), CYAN);
    renderAll();

    bool running = true;

    while (script.available() && running) {
        String line = script.readStringUntil('\n');
        executeScriptLine(line, running);
        M5Cardputer.update();
    }

    script.close();
}

static bool isTxtName(String name) {
    String n = name;
    n.toLowerCase();
    return n.endsWith(".txt");
}

static void createTextFile(String name) {
    if (currentSection != SD_SECTION) {
        addHistory("new only in sd", RED);
        return;
    }

    name.trim();
    if (!isTxtName(name)) {
        addHistory("use .txt", RED);
        return;
    }

    String path = buildSdPath(name);

    if (SD.exists(path)) {
        addHistory("file exists", YELLOW);
        return;
    }

    File f = SD.open(path, FILE_WRITE);

    if (!f) {
        addHistory("create fail", RED);
        return;
    }

    f.close();
    addHistory("created " + upperCopy(name), GREEN);
}

static void deleteTextFile(String name) {
    if (currentSection != SD_SECTION) {
        addHistory("del only in sd", RED);
        return;
    }

    name.trim();

    if (!isTxtName(name)) {
        addHistory("use .txt", RED);
        return;
    }

    String path = buildSdPath(name);

    if (!SD.exists(path)) {
        addHistory("file not found", RED);
        return;
    }

    File f = SD.open(path);

    if (f && f.isDirectory()) {
        f.close();
        addHistory("not a txt file", RED);
        return;
    }

    if (f) f.close();

    if (SD.remove(path)) {
        addHistory("deleted " + upperCopy(name), GREEN);
    } else {
        addHistory("delete fail", RED);
    }
}

static void renderEditor() {
    M5Cardputer.Display.fillScreen(BLACK);

    printAt(5, 4, "EDIT:", GREEN);
    printAt(45, 4, upperCopy(editorName), WHITE);

    int y = 18;
    int visibleLines = 7;

    if (editorScrollY < 0) editorScrollY = 0;
    if (editorScrollX < 0) editorScrollX = 0;

    int maxY = editorLines.size() > visibleLines ? editorLines.size() - visibleLines : 0;
    if (editorScrollY > maxY) editorScrollY = maxY;

    for (int i = 0; i < visibleLines; i++) {
        int lineIndex = editorScrollY + i;
        if (lineIndex >= editorLines.size()) break;

        String line = editorLines[lineIndex];

        if (editorScrollX < line.length()) {
            line = line.substring(editorScrollX);
        } else {
            line = "";
        }

        int maxChars = (SCREEN_W - 10) / CHAR_W;
        if (line.length() > maxChars) line = line.substring(0, maxChars);

        printAt(5, y, line, WHITE);
        y += LINE_H;
    }

    printAt(0, SEP_Y, "----------------------------------------", DARKGREY);

    if (editorSaveMode) {
        printAt(5, PROMPT_Y, "SAVE FILE?", YELLOW);
        printAt(90, PROMPT_Y, "OK SAVE", GREEN);
        printAt(160, PROMPT_Y, "ESC EXIT", WHITE);
    } else {
        printAt(5, PROMPT_Y, "OK line", WHITE);
        printAt(70, PROMPT_Y, "DEL", WHITE);
        printAt(110, PROMPT_Y, "FN scroll", WHITE);
        printAt(185, PROMPT_Y, "ESC", YELLOW);
    }
}

static void saveEditorFile() {
    SD.remove(editorPath);

    File f = SD.open(editorPath, FILE_WRITE);

    if (!f) {
        editorMode = false;
        editorSaveMode = false;
        addHistory("save fail", RED);
        renderAll();
        return;
    }

    for (size_t i = 0; i < editorLines.size(); i++) {
        f.print(editorLines[i]);
        if (i + 1 < editorLines.size()) f.print("\n");
    }

    f.close();

    editorMode = false;
    editorSaveMode = false;

    addHistory("saved " + upperCopy(editorName), GREEN);
    renderAll();
}

static void openTextEditor(String name) {
    if (currentSection != SD_SECTION) {
        addHistory("open only in sd", RED);
        return;
    }

    name.trim();
    if (!isTxtName(name)) {
        addHistory("use .txt", RED);
        return;
    }

    String path = buildSdPath(name);

    if (!SD.exists(path)) {
        addHistory("file not found", RED);
        return;
    }

    File f = SD.open(path, FILE_READ);

    if (!f || f.isDirectory()) {
        addHistory("open fail", RED);
        if (f) f.close();
        return;
    }

    editorLines.clear();
    editorPath = path;
    editorName = name;
    editorScrollX = 0;
    editorScrollY = 0;
    editorSaveMode = false;

    while (f.available() && editorLines.size() < EDITOR_MAX_LINES) {
        String line = f.readStringUntil('\n');
        line.replace("\r", "");

        if (line.length() > EDITOR_MAX_COLS) {
            line = line.substring(0, EDITOR_MAX_COLS);
        }

        editorLines.push_back(line);
    }

    f.close();

    if (editorLines.size() == 0) {
        editorLines.push_back("");
    }

    editorMode = true;
    renderEditor();
}

static void editorNewLine() {
    if (editorLines.size() < EDITOR_MAX_LINES) {
        editorLines.push_back("");
        editorScrollY = editorLines.size() > 7 ? editorLines.size() - 7 : 0;
    }

    renderEditor();
}

static void editorBackspace() {
    if (editorLines.size() == 0) return;

    String& line = editorLines.back();

    if (line.length() > 0) {
        line.remove(line.length() - 1);
    } else if (editorLines.size() > 1) {
        editorLines.pop_back();
    }

    renderEditor();
}

static void editorAddChar(char c) {
    if (editorLines.size() == 0) editorLines.push_back("");

    String& line = editorLines.back();

    if (line.length() < EDITOR_MAX_COLS) {
        line += c;

        int visibleCols = (SCREEN_W - 10) / CHAR_W;
        if (line.length() > visibleCols) {
            editorScrollX = line.length() - visibleCols;
        }
    }

    renderEditor();
}

static void editorScrollUp() {
    editorScrollY--;
    renderEditor();
}

static void editorScrollDown() {
    editorScrollY++;
    renderEditor();
}

static void editorScrollLeft() {
    editorScrollX--;
    renderEditor();
}

static void editorScrollRight() {
    editorScrollX++;
    renderEditor();
}

static void editorExitNoSave() {
    editorMode = false;
    editorSaveMode = false;
    addHistory("edit closed", DARKGREY);
    renderAll();
}

static void executeCommand(String cmd) {
    cmd.trim();
    cmd.toLowerCase();

    addHistory(prompt() + inputLine, GREEN);

    if (cmd == "dir") {
        if (currentSection == ROOT) showRootDir();
        else if (currentSection == OPTION) showOptionDir();
        else if (currentSection == SD_SECTION) showSdDir();

    } else if (cmd == "help") {
        inputLine = "";
        commandOffset = 0;
        helpScroll = 0;
        showHelpScreen();
        return;

    } else if (cmd == "info") {
        showInfo();

    } else if (cmd == "clean" || cmd == "clian") {
        clearShell();
        return;

    } else if (cmd == "run") {
        runScript("");

    } else if (cmd.startsWith("run ")) {
        runScript(cmd.substring(4));

    } else if (cmd.startsWith("new ")) {
        createTextFile(cmd.substring(4));

    } else if (cmd.startsWith("open ")) {
        openTextEditor(cmd.substring(5));
        inputLine = "";
        autoScrollCommandEnd();
        return;

    } else if (cmd.startsWith("del ")) {
        deleteTextFile(cmd.substring(4));

    } else if (cmd == "cd .." || cmd == "cd..") {
        if (currentSection == SD_SECTION) sdGoBack();
        else currentSection = ROOT;
        addPathLine();

    } else if (cmd == "cd option" || cmd == "cd options") {
        if (currentSection == ROOT) {
            currentSection = OPTION;
            addPathLine();
            inputLine = "";
            commandOffset = 0;
            showOptionsScreen();
            return;
        } else {
            addHistory("bad path", RED);
        }

    } else if (cmd == "cd sd") {
        if (currentSection == ROOT) {
            currentSection = SD_SECTION;
            sdPath = "/";
            addPathLine();
        } else {
            addHistory("bad path", RED);
        }

    } else if (cmd.startsWith("cd ") && currentSection == SD_SECTION) {
        String name = cmd.substring(3);
        name.trim();

        String nextPath = buildSdPath(name);

        File dir = SD.open(nextPath);

        if (dir && dir.isDirectory()) {
            sdPath = nextPath;
            addPathLine();
        } else {
            addHistory("bad path", RED);
        }

        if (dir) dir.close();

    } else if (cmd == "") {
        // empty OK

    } else {
        addHistory("unknown command", RED);
    }

    inputLine = "";
    autoScrollCommandEnd();
    renderAll();
}

static void scrollHistoryUp() {
    scrollOffset++;
    renderAll();
}

static void scrollHistoryDown() {
    scrollOffset--;
    renderAll();
}

static void scrollCommandLeft() {
    commandOffset--;
    drawBottomBar();
}

static void scrollCommandRight() {
    commandOffset++;
    drawBottomBar();
}

void shellStart() {
    currentSection = ROOT;
    sdPath = "/";
    inputLine = "";
    commandOffset = 0;
    scrollOffset = 0;
    helpScroll = 0;
    helpMode = false;
    optionsMode = false;
    editorMode = false;
    editorSaveMode = false;

    updateBattery(true);
    applyBrightness();
    applyVolume(false);

    clearShell();
}

void shellUpdate() {
    if (updateBattery(false)) {
        drawBattery();
    }

    if (!M5Cardputer.Keyboard.isChange() ||
        !M5Cardputer.Keyboard.isPressed()) {
        return;
    }

    auto status = M5Cardputer.Keyboard.keysState();

    if (editorMode) {
        if (editorSaveMode) {
            if (status.enter) {
                saveEditorFile();
                return;
            }

            if (status.esc) {
                editorExitNoSave();
                return;
            }

            return;
        }

        if (status.esc) {
            editorSaveMode = true;
            renderEditor();
            return;
        }

        if (status.enter) {
            editorNewLine();
            return;
        }

        if (status.backspace) {
            editorBackspace();
            return;
        }

        if (status.up) {
            editorScrollUp();
            return;
        }

        if (status.down) {
            editorScrollDown();
            return;
        }

        if (status.left) {
            editorScrollLeft();
            return;
        }

        if (status.right) {
            editorScrollRight();
            return;
        }

        for (auto c : status.word) {
            if (c >= 32 && c <= 126) {
                editorAddChar(c);
            }
        }

        return;
    }

    if (optionsMode) {
        if (status.esc) {
            exitOptions();
            return;
        }

        if (status.up) {
            optionsMoveUp();
            return;
        }

        if (status.down) {
            optionsMoveDown();
            return;
        }

        if (status.left) {
            optionsAdjustLeft();
            return;
        }

        if (status.right) {
            optionsAdjustRight();
            return;
        }

        return;
    }

    if (helpMode) {
        if (status.esc) {
            helpMode = false;
            renderAll();
            return;
        }

        if (status.up) {
            helpScroll--;
            showHelpScreen();
            return;
        }

        if (status.down) {
            helpScroll++;
            showHelpScreen();
            return;
        }

        return;
    }

    if (status.up) {
        scrollHistoryUp();
        return;
    }

    if (status.down) {
        scrollHistoryDown();
        return;
    }

    if (status.left) {
        scrollCommandLeft();
        return;
    }

    if (status.right) {
        scrollCommandRight();
        return;
    }

    if (status.enter) {
        executeCommand(inputLine);
        return;
    }

    if (status.backspace) {
        if (inputLine.length() > 0) {
            inputLine.remove(inputLine.length() - 1);
            autoScrollCommandEnd();
            drawBottomBar();
        }

        return;
    }

    for (auto c : status.word) {
        if (c >= 32 && c <= 126) {
            inputLine += c;
            autoScrollCommandEnd();
            drawBottomBar();
        }
    }
}