#include "shell_internal.h"

// STRING UTILS
// ───────────────────────────────────────────────────────────────────
String upperCopy(const String& s) {
    String r = s;
    r.toUpperCase();
    return r;
}

String lowerCopy(const String& s) {
    String r = s;
    r.toLowerCase();
    return r;
}

String padRight(const String& s, int w) {
    String r = s;
    while ((int)r.length() < w) r += ' ';
    return r;
}

String cleanName(const String& path) {
    int slash = path.lastIndexOf('/');
    return (slash >= 0) ? path.substring(slash + 1) : path;
}

String trimToWidth(const String& s, int maxChars) {
    if ((int)s.length() <= maxChars) return s;
    if (maxChars <= 3) return s.substring(0, maxChars);
    return "..." + s.substring(s.length() - (maxChars - 3));
}

bool isSafeName(const String& name) {
    if (name.length() == 0) return false;
    if (name == ".") return false;
    if (name == "..") return false;
    if (name.indexOf("..") >= 0) return false;
    if (name.indexOf('/') >= 0) return false;
    if (name.indexOf('\\') >= 0) return false;
    return true;
}

bool buildSdPath(const String& name, String& out) {
    if (!isSafeName(name)) return false;
    out = (S.sdPath == "/") ? "/" + name : S.sdPath + "/" + name;
    return true;
}

bool isHiddenEntry(const String& raw) {
    String n = lowerCopy(raw);
    return n == "system volume information"
        || n == ".tmp_rec.wav"
        || n == ".trashes"
        || n == ".spotlight-v100"
        || n == ".fseventsd"
        || n == "recycler"
        || n == "$recycle.bin";
}

bool isEditable(const String& name) {
    String n = lowerCopy(name);
    return n.endsWith(".txt") || n.endsWith(".id");
}

bool isAudioFile(const String& name) {
    String n = lowerCopy(name);
    return n.endsWith(".wav");
}

bool looksLikeFileName(const String& name) {
    int dot = name.lastIndexOf('.');
    return dot > 0 && dot < (int)name.length() - 1;
}

// ───────────────────────────────────────────────────────────────────
// DISPLAY HELPERS
// ───────────────────────────────────────────────────────────────────
void setTxtColor(uint16_t fg) {
    M5Cardputer.Display.setTextSize(Dim::TEXT_SIZE);
    M5Cardputer.Display.setTextColor(fg, BLACK);
}

void printAt(int x, int y, const String& t, uint16_t fg) {
    setTxtColor(fg);
    M5Cardputer.Display.setCursor(x, y);
    M5Cardputer.Display.print(t);
}

String prompt() {
    if (S.section == ROOT) return "id:\\>";
    if (S.section == OPTION) return "opt:\\>";
    if (S.section == LAB_SECTION) return "lab:\\>";
    if (S.sdPath == "/") return "sd:\\>";

    String p = S.sdPath;
    p.replace("/", "\\");
    return "sd:" + p + "\\>";
}

String breadcrumb() {
    if (S.section == ROOT) return "id:\\>";
    if (S.section == OPTION) return "id:\\>OPT\\";
    if (S.section == LAB_SECTION) return "id:\\>LAB\\";

    String p = "id:\\>SD";
    if (S.sdPath != "/") {
        String s = S.sdPath;
        s.replace("/", "\\");
        p += s;
    }
    p += "\\";
    return p;
}

String uiPath() {
    if (S.uiMode == UI_LAB) return "id:\\>LAB\\";
    if (S.uiMode == UI_WLAN_LIST) return "id:\\>LAB\\WLAN\\";
    if (S.uiMode == UI_WLAN_INFO) return "id:\\>LAB\\WLAN\\INFO\\";
    if (S.uiMode == UI_WLAN_PASS) return "id:\\>LAB\\WLAN\\PASS\\";
    if (S.uiMode == UI_BLE_INFO) return "id:\\>LAB\\BLE\\";
    if (S.uiMode == UI_BLE_NAME_EDIT) return "id:\\>LAB\\BLE\\NAME\\";
    if (S.uiMode == UI_AUDIO_RECORDER) return "id:\\>LAB\\REC\\";
    if (S.uiMode == UI_AUDIO_SAVE_MENU) return "id:\\>LAB\\REC\\SAVE\\";
    if (S.uiMode == UI_AUDIO_NAME_EDIT) return "id:\\>LAB\\REC\\NAME\\";
    if (S.uiMode == UI_AUDIO_PLAYER) return "id:\\>SD\\AUDIO\\PLAYER\\";
    if (S.uiMode == UI_HW_TOOLS) return "id:\\>LAB\\TOOLS\\";
    if (S.uiMode == UI_BALL_APP) return "id:\\>LAB\\BALL\\";
    if (S.uiMode == UI_OPTIONS || S.uiMode == UI_CONFIG_EDIT) return "id:\\>OPT\\";
    return breadcrumb();
}

// ───────────────────────────────────────────────────────────────────
// BRIGHTNESS / VOLUME
// ───────────────────────────────────────────────────────────────────
int fullBrightness() {
    return map(S.brightness, 0, Limit::OPTION_MAX, 20, 255);
}

void applyFullBrightness() {
    M5Cardputer.Display.setBrightness(fullBrightness());
}

void applyVolume(bool beep) {
    int vol = map(S.volume, 0, Limit::OPTION_MAX, 0, 255);
    soundSetVolume((uint8_t)vol);

    if (beep && S.volume > 0 && soundIsEnabled()) {
        int hz = 480 + S.volume * 110;
        M5Cardputer.Speaker.tone(hz, 70);
    }
}

// ───────────────────────────────────────────────────────────────────
// BATTERY / STATUS BAR
// ───────────────────────────────────────────────────────────────────
int readBatPct() {
    int v = M5Cardputer.Power.getBatteryLevel();
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}

uint16_t batColor(int pct) {
    if (pct <= 20) return RED;
    if (pct <= 50) return YELLOW;
    return GREEN;
}

String batStr(int pct) {
    int bars = 0;
    if      (pct > 80) bars = 5;
    else if (pct > 60) bars = 4;
    else if (pct > 40) bars = 3;
    else if (pct > 20) bars = 2;
    else if (pct > 5)  bars = 1;

    String s = "[";
    for (int i = 0; i < 5; i++) s += (i < bars) ? '|' : ' ';
    s += "] ";
    s += String(pct);
    s += "%";
    return s;
}

void drawStatusBar() {
    M5Cardputer.Display.fillRect(0, Dim::STATUS_Y, Dim::SCREEN_W, Dim::STATUS_H, BLACK);

    S.batPct = readBatPct();

    String path = trimToWidth(uiPath(), 22);
    printAt(Dim::X0, 3, path, GREEN);

    printAt(148, 3, batStr(S.batPct), batColor(S.batPct));

    if (networkConnected()) {
    M5Cardputer.Display.fillCircle(234, 7, 2, WHITE);
}
}

// ───────────────────────────────────────────────────────────────────
// HISTORY / RENDER
// ───────────────────────────────────────────────────────────────────
int visibleRows() {
    return (Dim::SEP_Y - Dim::HIST_Y0) / Dim::LINE_H;
}

void pushHistoryLine(const String& text, uint16_t color) {
    TermLine line;
    line.text = text;
    line.color = color;
    S.history.push_back(line);
}

void addHistory(const String& text, uint16_t color) {
    if ((int)S.history.size() >= Limit::MAX_HISTORY) {
        S.history.erase(S.history.begin());

        if (!S.historyWarned) {
            S.historyWarned = true;

            if ((int)S.history.size() >= Limit::MAX_HISTORY) {
                S.history.erase(S.history.begin());
            }
            pushHistoryLine("HISTORY LIMIT REACHED", YELLOW);

            if ((int)S.history.size() >= Limit::MAX_HISTORY) {
                S.history.erase(S.history.begin());
            }
            pushHistoryLine("USE CLS TO CLEAR HISTORY", DARKGREY);
        }
    }

    if ((int)S.history.size() >= Limit::MAX_HISTORY) {
        S.history.erase(S.history.begin());
    }

    pushHistoryLine(text, color);
    S.scrollOffset = 0;
}

void addHelpRow(const String& cmd, const String& desc)
{
    if ((int)S.history.size() >= Limit::MAX_HISTORY) {
        S.history.erase(S.history.begin());
    }

    TermLine line;
    line.split = true;
    line.left = cmd;
    line.right = desc;
    line.leftColor = GREEN;
    line.rightColor = WHITE;

    S.history.push_back(line);
    S.scrollOffset = 0;
}

void addBreadcrumb() {
    addHistory(breadcrumb(), GREEN);
}

void drawHistoryScrollbar(int total, int visible, int maxScr) {
    if (total <= visible) return;

    int x = Dim::SCROLL_X;
    int y = Dim::HIST_Y0;
    int h = Dim::SEP_Y - Dim::HIST_Y0 - 2;

    M5Cardputer.Display.drawRect(x, y, 3, h, DARKGREY);

    int thumbH = (h * visible) / total;
    if (thumbH < 8) thumbH = 8;
    if (thumbH > h - 2) thumbH = h - 2;

    int thumbY = y + 1;
    if (maxScr > 0) {
        thumbY = y + 1 + ((h - 2 - thumbH) * (maxScr - S.scrollOffset)) / maxScr;
    }

    M5Cardputer.Display.fillRect(x + 1, thumbY, 1, thumbH, GREEN);
}

void renderHistory() {
    M5Cardputer.Display.fillRect(0, 0, Dim::SCREEN_W, Dim::SEP_Y, BLACK);
    drawStatusBar();

    int vis = visibleRows();
    int total = (int)S.history.size();
    int maxScr = (total > vis) ? total - vis : 0;

    if (S.scrollOffset < 0) S.scrollOffset = 0;
    if (S.scrollOffset > maxScr) S.scrollOffset = maxScr;

    int start = total - vis - S.scrollOffset;
    if (start < 0) start = 0;

    int y = Dim::HIST_Y0;
    int maxTextChars = (Dim::SCROLL_X - Dim::X0 - 2) / Dim::CHAR_W;

for (int i = start; i < total && y < Dim::SEP_Y - Dim::LINE_H + 1; i++) {
    if (S.history[i].split) {
        printAt(Dim::X0, y, S.history[i].left, S.history[i].leftColor);
        printAt(58, y, S.history[i].right, S.history[i].rightColor);
    } else {
        printAt(Dim::X0, y, trimToWidth(S.history[i].text, maxTextChars), S.history[i].color);
    }

    y += Dim::LINE_H;
}

    drawHistoryScrollbar(total, vis, maxScr);
}

static bool cmdCursorOn()
{
    return ((millis() / 450UL) & 1UL) == 0;
}

String cmdLine()
{
    return prompt() + S.inputLine + (cmdCursorOn() ? "_" : " ");
}

static String cmdLineBase()
{
    return prompt() + S.inputLine;
}

int maxCmdChars() {
    return (Dim::SCREEN_W - Dim::X0) / Dim::CHAR_W;
}

void autoCmdEnd() {
    int max = maxCmdChars();
    int total = (int)cmdLineBase().length() + 1;
    S.cmdOffset = (total > max) ? total - max : 0;
}

void clampCmdOffset() {
    int max = maxCmdChars();
    int total = (int)cmdLineBase().length() + 1;

    if (S.cmdOffset < 0) S.cmdOffset = 0;
    if (total <= max) {
        S.cmdOffset = 0;
        return;
    }

    int cap = total - max;
    if (S.cmdOffset > cap) S.cmdOffset = cap;
}

void drawCmdLine()
{
    clampCmdOffset();

    // Clear only the editable command row, not the separator/footer.
    // This prevents the grey lower separator from blinking with the cursor.
    M5Cardputer.Display.fillRect(0, Dim::PROMPT_Y, Dim::SCREEN_W, Dim::SCREEN_H - Dim::PROMPT_Y, BLACK);

    String base = cmdLineBase();
    int max = maxCmdChars();
    int pLen = (int)prompt().length();
    int cursorIndex = (int)base.length();
    int total = cursorIndex + 1;

    int end = S.cmdOffset + max;
    if (end > total) end = total;

    int x = Dim::X0;
    for (int i = S.cmdOffset; i < end; i++) {
        char ch = (i == cursorIndex) ? (cmdCursorOn() ? '_' : ' ') : base[i];
        uint16_t col = (i < pLen || i == cursorIndex) ? GREEN : WHITE;
        printAt(x, Dim::PROMPT_Y, String(ch), col);
        x += Dim::CHAR_W;
    }
}

void drawCmdCursorOnly()
{
    clampCmdOffset();

    String base = cmdLineBase();
    int cursorIndex = (int)base.length();
    int max = maxCmdChars();

    if (cursorIndex < S.cmdOffset || cursorIndex >= S.cmdOffset + max) return;

    int x = Dim::X0 + (cursorIndex - S.cmdOffset) * Dim::CHAR_W;
    M5Cardputer.Display.fillRect(x, Dim::PROMPT_Y, Dim::CHAR_W, Dim::LINE_H, BLACK);
    printAt(x, Dim::PROMPT_Y, cmdCursorOn() ? "_" : " ", GREEN);
}

void drawBottomBar() {
    M5Cardputer.Display.fillRect(0, Dim::SEP_Y, Dim::SCREEN_W, Dim::SCREEN_H - Dim::SEP_Y, BLACK);
    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    drawCmdLine();
}

void renderAll() {
    renderHistory();
    drawBottomBar();
}

void clearShell() {
    S.history.clear();
    S.historyWarned = false;
    S.scrollOffset = 0;
    S.cmdOffset = 0;
    S.inputLine = "";
    addBreadcrumb();
    M5Cardputer.Display.fillScreen(BLACK);
    renderAll();
}

// ───────────────────────────────────────────────────────────────────
