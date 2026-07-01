#include "shell_internal.h"

// TEXT EDITOR
// ───────────────────────────────────────────────────────────────────

static String editorReadLimitedLine(File& f, size_t limit)
{
    String out;
    out.reserve(limit < 96 ? limit : 96);
    while (f.available()) {
        char c = (char)f.read();
        if (c == '\n') break;
        if (c == '\r') continue;
        if (out.length() < limit) out += c;
    }
    return out;
}

int editorVisibleRows() {
    return (Dim::SEP_Y - Dim::HIST_Y0) / Dim::LINE_H;
}

String lineNumber(int index) {
    int n = index + 1;
    if (n < 10) return "00" + String(n);
    if (n < 100) return "0" + String(n);
    return String(n);
}

int editorTextRows() {
    int rows = editorVisibleRows() - 1;
    return (rows > 0) ? rows : 1;
}

int editorVisibleCols() {
    int cols = (Dim::SCREEN_W - Dim::X0 - 30) / Dim::CHAR_W;
    return (cols > 0) ? cols : 1;
}

int editorMaxScrollY() {
    int max = (int)editorLines.size() - editorTextRows();
    return (max > 0) ? max : 0;
}

int editorMaxScrollX() {
    int max = 0;
    for (const String& ln : editorLines) {
        if ((int)ln.length() > max) max = ln.length();
    }

    int visibleCols = editorVisibleCols();
    int cap = max - visibleCols;
    return (cap > 0) ? cap : 0;
}

void clampEditorCursor() {
    if (editorLines.empty()) editorLines.push_back("");

    if (editorCursorY < 0) editorCursorY = 0;
    if (editorCursorY >= (int)editorLines.size()) editorCursorY = (int)editorLines.size() - 1;

    int len = editorLines[editorCursorY].length();
    if (editorCursorX < 0) editorCursorX = 0;
    if (editorCursorX > len) editorCursorX = len;
}

void clampEditorScroll() {
    if (editorScrollY < 0) editorScrollY = 0;
    if (editorScrollY > editorMaxScrollY()) editorScrollY = editorMaxScrollY();

    if (editorScrollX < 0) editorScrollX = 0;
    if (editorScrollX > editorMaxScrollX()) editorScrollX = editorMaxScrollX();
}

void editorFollowCursor() {
    clampEditorCursor();

    int rows = editorTextRows();
    int cols = editorVisibleCols();

    if (editorCursorY < editorScrollY) editorScrollY = editorCursorY;
    if (editorCursorY >= editorScrollY + rows) editorScrollY = editorCursorY - rows + 1;

    if (editorCursorX < editorScrollX) editorScrollX = editorCursorX;
    if (editorCursorX >= editorScrollX + cols) editorScrollX = editorCursorX - cols + 1;

    clampEditorScroll();
}

void renderEditorExitMenu() {
    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "EXIT EDITOR", CYAN);

    const char* items[] = { "SAVE", "NO SAVE", "CANCEL" };
    for (int i = 0; i < 3; i++) {
        int y = 45 + i * 17;
        bool sel = (editorSaveSelection == i);
        printAt(5, y, sel ? ">" : " ", GREEN);
        printAt(18, y, items[i], sel ? GREEN : WHITE);
    }

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER choose  ESC cancel", DARKGREY);
}

void renderEditor() {
    if (editorSaveMode) {
        renderEditorExitMenu();
        return;
    }

    if (editorLines.empty()) editorLines.push_back("");
    editorFollowCursor();

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(Dim::X0, Dim::HIST_Y0, trimToWidth("OPEN " + editorName, 30), CYAN);

    int y = Dim::HIST_Y0 + Dim::LINE_H;
    int rows = editorTextRows();
    int visibleCols = editorVisibleCols();

    for (int row = 0; row < rows; row++) {
        int idx = editorScrollY + row;
        if (idx >= (int)editorLines.size()) break;

        String ln = editorLines[idx];

        if (editorScrollX < (int)ln.length()) ln = ln.substring(editorScrollX);
        else ln = "";

        if ((int)ln.length() > visibleCols) ln = ln.substring(0, visibleCols);

        bool activeLine = (idx == editorCursorY);
        printAt(Dim::X0, y, lineNumber(idx), activeLine ? GREEN : DARKGREY);
        printAt(Dim::X0 + 24, y, ln, activeLine ? WHITE : DARKGREY);
        y += Dim::LINE_H;
    }

    drawMenuScrollbar((int)editorLines.size(), rows, editorScrollY, Dim::HIST_Y0 + Dim::LINE_H, Dim::SEP_Y - Dim::HIST_Y0 - Dim::LINE_H - 1);

    M5Cardputer.Display.fillRect(0, Dim::SEP_Y, Dim::SCREEN_W, Dim::SCREEN_H - Dim::SEP_Y, BLACK);
    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "FN+ESC menu  ARROWS move", DARKGREY);

    drawEditorCursorOnly();
}


static bool editorCursorOn()
{
    return ((millis() / 450UL) & 1UL) == 0;
}

void drawEditorCursorOnly()
{
    if (!editorMode || editorSaveMode) return;
    if (editorLines.empty()) return;

    clampEditorCursor();
    clampEditorScroll();

    int row = editorCursorY - editorScrollY;
    int rows = editorTextRows();
    if (row < 0 || row >= rows) return;

    int col = editorCursorX - editorScrollX;
    int visibleCols = editorVisibleCols();
    if (col < 0 || col >= visibleCols) return;

    int x = Dim::X0 + 24 + col * Dim::CHAR_W;
    int y = Dim::HIST_Y0 + Dim::LINE_H + row * Dim::LINE_H;

    M5Cardputer.Display.fillRect(x, y, Dim::CHAR_W, Dim::LINE_H, BLACK);

    if (editorCursorOn()) {
        printAt(x, y, "_", GREEN);
        return;
    }

    String& ln = editorLines[editorCursorY];
    if (editorCursorX < (int)ln.length()) {
        printAt(x, y, String(ln[editorCursorX]), WHITE);
    } else {
        printAt(x, y, " ", WHITE);
    }
}

bool saveEditorFile() {
    if (editorPath.length() == 0) return false;

    if (SD.exists(editorPath)) SD.remove(editorPath);

    File f = SD.open(editorPath, FILE_WRITE);
    if (!f) return false;

    for (size_t i = 0; i < editorLines.size(); i++) {
        f.print(editorLines[i]);
        if (i + 1 < editorLines.size()) f.print('\n');
    }

    f.close();
    return true;
}

void editorExitNoSave() {
    editorMode = false;
    editorSaveMode = false;
    editorSaveSelection = 0;
    editorPath = "";
    editorName = "";
    editorScrollY = 0;
    editorScrollX = 0;
    editorCursorY = 0;
    editorCursorX = 0;
    editorLines.clear();

    S.uiMode = UI_SHELL;
    S.inputLine = "";
    S.cmdOffset = 0;
    renderAll();
}

void editorNewLine() {
    if ((int)editorLines.size() >= Limit::EDITOR_MAX_LINES) {
        soundError();
        return;
    }

    if (editorLines.empty()) editorLines.push_back("");
    clampEditorCursor();

    String tail = editorLines[editorCursorY].substring(editorCursorX);
    editorLines[editorCursorY].remove(editorCursorX);
    editorLines.insert(editorLines.begin() + editorCursorY + 1, tail);

    editorCursorY++;
    editorCursorX = 0;
    editorFollowCursor();
    renderEditor();
}

void editorBackspace() {
    if (editorLines.empty()) editorLines.push_back("");
    clampEditorCursor();

    if (editorCursorX > 0) {
        String& ln = editorLines[editorCursorY];
        ln.remove(editorCursorX - 1, 1);
        editorCursorX--;
    } else if (editorCursorY > 0) {
        int prevLen = editorLines[editorCursorY - 1].length();
        editorLines[editorCursorY - 1] += editorLines[editorCursorY];
        editorLines.erase(editorLines.begin() + editorCursorY);
        editorCursorY--;
        editorCursorX = prevLen;
    } else {
        return;
    }

    editorFollowCursor();
    renderEditor();
}

void editorAddChar(char c) {
    if (editorLines.empty()) editorLines.push_back("");
    clampEditorCursor();

    String& ln = editorLines[editorCursorY];
    if ((int)ln.length() >= Limit::EDITOR_MAX_COLS) {
        soundError();
        return;
    }

    ln = ln.substring(0, editorCursorX) + String(c) + ln.substring(editorCursorX);
    editorCursorX++;

    editorFollowCursor();
    renderEditor();
}

void editorMoveUp() {
    if (editorCursorY > 0) {
        editorCursorY--;
        clampEditorCursor();
    }
    editorFollowCursor();
    renderEditor();
}

void editorMoveDown() {
    if (editorCursorY + 1 < (int)editorLines.size()) {
        editorCursorY++;
        clampEditorCursor();
    }
    editorFollowCursor();
    renderEditor();
}

void editorMoveLeft() {
    if (editorCursorX > 0) {
        editorCursorX--;
    } else if (editorCursorY > 0) {
        editorCursorY--;
        editorCursorX = editorLines[editorCursorY].length();
    }
    editorFollowCursor();
    renderEditor();
}

void editorMoveRight() {
    if (editorLines.empty()) editorLines.push_back("");
    clampEditorCursor();

    int len = editorLines[editorCursorY].length();
    if (editorCursorX < len) {
        editorCursorX++;
    } else if (editorCursorY + 1 < (int)editorLines.size()) {
        editorCursorY++;
        editorCursorX = 0;
    }
    editorFollowCursor();
    renderEditor();
}

void editorOpenExitMenu() {
    editorSaveMode = true;
    editorSaveSelection = 0;
    renderEditorExitMenu();
}

void openTextEditor(String name) {
    if (S.section != SD_SECTION) {
        addHistory("OPEN ONLY IN SD", RED);
        soundError();
        return;
    }

    name.trim();

    String path;
    if (!buildSdPath(name, path)) {
        addHistory("INVALID NAME", RED);
        soundError();
        return;
    }

    if (isAudioFile(name)) {
        openAudioPlayerByPath(path, cleanName(path));
        return;
    }

    if (!isEditable(name)) {
        addHistory("NO VIEWER", RED);
        addHistory("OPEN .TXT .ID .WAV", DARKGREY);
        soundError();
        return;
    }

    File f = SD.open(path, FILE_READ);
    if (!f || f.isDirectory()) {
        addHistory("OPEN FAIL", RED);
        if (f) f.close();
        soundError();
        return;
    }

    editorLines.clear();
    editorLines.reserve(Limit::EDITOR_MAX_LINES);

    while (f.available() && (int)editorLines.size() < Limit::EDITOR_MAX_LINES) {
        String ln = editorReadLimitedLine(f, Limit::EDITOR_MAX_COLS);
        editorLines.push_back(ln);
    }

    f.close();

    if (editorLines.empty()) editorLines.push_back("");

    editorPath = path;
    editorName = cleanName(path);
    editorCursorY = 0;
    editorCursorX = 0;
    editorScrollY = 0;
    editorScrollX = 0;
    editorSaveMode = false;
    editorSaveSelection = 0;
    editorMode = true;
    S.uiMode = UI_SHELL;

    M5Cardputer.Display.fillScreen(BLACK);
    renderEditor();
}

// ───────────────────────────────────────────────────────────────────
