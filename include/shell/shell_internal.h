#pragma once
#include <M5Cardputer.h>
#include <SD.h>
#include <vector>

#include "shell.h"
#include "storage.h"
#include "sound.h"
#include "config.h"
#include "network.h"
#include "bluetooth_lab.h"
#include "audio_lab.h"
#include "hardware_tools.h"
#include "system_log.h"
#include "version.h"

namespace Dim {
    constexpr int SCREEN_W  = 240;
    constexpr int SCREEN_H  = 135;
    constexpr int TEXT_SIZE = 1;
    constexpr int CHAR_W    = 6;
    constexpr int LINE_H    = 13;
    constexpr int X0        = 4;
    constexpr int STATUS_Y  = 0;
    constexpr int STATUS_H  = 15;
    constexpr int HIST_Y0   = 17;
    constexpr int SEP_Y     = 112;
    constexpr int PROMPT_Y  = 121;
    constexpr int SCROLL_X  = 236;
}

namespace Limit {
    constexpr int MAX_HISTORY       = 300;
    constexpr int OPTION_MAX        = 6;
    constexpr int CAT_LINES         = 20;
    constexpr int CAT_LEN           = 34;
    constexpr int INPUT_MAX         = 128;
    constexpr int PASSWORD_MAX      = 64;
    constexpr int OPT_TOTAL         = 5;
    constexpr int OPT_VISIBLE       = 5;
    constexpr int LAB_TOTAL         = 5;
    constexpr int LAB_VISIBLE       = 4;
    constexpr int WLAN_VISIBLE      = 5;
    constexpr int WLAN_INFO_VISIBLE = 5;
    constexpr int EDITOR_MAX_LINES  = 300;
    constexpr int EDITOR_MAX_COLS   = 300;
}

enum Section {
    ROOT,
    OPTION,
    LAB_SECTION,
    SD_SECTION
};

enum EditKind {
    EDIT_NONE,
    EDIT_USERNAME,
    EDIT_PASSWORD
};

enum UiMode {
    UI_SHELL,
    UI_OPTIONS,
    UI_CONFIG_EDIT,
    UI_LAB,
    UI_WLAN_LIST,
    UI_WLAN_INFO,
    UI_WLAN_PASS,
    UI_BLE_INFO,
    UI_BLE_NAME_EDIT,
    UI_AUDIO_RECORDER,
    UI_AUDIO_SAVE_MENU,
    UI_AUDIO_NAME_EDIT,
    UI_AUDIO_PLAYER,
    UI_HW_TOOLS,
    UI_BALL_APP
};

struct TermLine {
    String text;
    uint16_t color = WHITE;
    bool split = false;
    String left = "";
    String right = "";
    uint16_t leftColor = GREEN;
    uint16_t rightColor = WHITE;
};

struct ShellState {
    Section section = ROOT;
    String sdPath = "/";
    UiMode uiMode = UI_SHELL;
    bool welcomeActive = true;
    String inputLine = "";
    int scrollOffset = 0;
    int cmdOffset = 0;
    std::vector<TermLine> history;
    bool historyWarned = false;
    int optSel = 0;
    int optScroll = 0;
    int brightness = 4;
    int volume = 3;
    EditKind editKind = EDIT_NONE;
    String editBuffer = "";
    int labSel = 0;
    int labScroll = 0;
    int bleMenuSel = 0;
    String bleNameEdit = "";
    int wlanSel = 0;
    int wlanScroll = 0;
    int wlanCount = 0;
    int wlanInfoScroll = 0;
    int selectedNet = -1;
    String wlanPassword = "";
    bool clipActive = false;
    String clipPath = "";
    String clipName = "";
    bool clipIsDir = false;
    int audioSaveSel = 0;
    String audioNameEdit = "";
    String audioPlayerPath = "";
    String audioPlayerName = "";
    uint32_t audioLastDrawMs = 0;
    int batPct = 0;
    uint32_t batLastMs = 0;
};

extern ShellState S;
extern bool editorMode;
extern bool editorSaveMode;
extern int editorSaveSelection;
extern std::vector<String> editorLines;
extern String editorPath;
extern String editorName;
extern int editorScrollY;
extern int editorScrollX;
extern int editorCursorY;
extern int editorCursorX;
extern int audioWave[120];
extern int audioWavePos;
extern uint32_t audioLastWaveMs;
extern bool lockRequested;

// string/render helpers
String upperCopy(const String& s);
String lowerCopy(const String& s);
String padRight(const String& s, int w);
String cleanName(const String& path);
String trimToWidth(const String& s, int maxChars);
bool isSafeName(const String& name);
bool buildSdPath(const String& name, String& out);
bool isHiddenEntry(const String& raw);
bool isEditable(const String& name);
bool isAudioFile(const String& name);
bool looksLikeFileName(const String& name);
void setTxtColor(uint16_t fg);
void printAt(int x, int y, const String& t, uint16_t fg = WHITE);
String prompt();
String breadcrumb();
String uiPath();
int fullBrightness();
void applyFullBrightness();
void applyVolume(bool beep = true);
int readBatPct();
uint16_t batColor(int pct);
String batStr(int pct);
void drawStatusBar();
int visibleRows();
void pushHistoryLine(const String& text, uint16_t color);
void addHistory(const String& text = "", uint16_t color = WHITE);
void addHelpRow(const String& cmd, const String& desc);
void addBreadcrumb();
void drawHistoryScrollbar(int total, int visible, int maxScr);
void renderHistory();
String cmdLine();
int maxCmdChars();
void autoCmdEnd();
void clampCmdOffset();
void drawCmdLine();
void drawCmdCursorOnly();
void drawBottomBar();
void renderAll();
void clearShell();

// screens/menus
void showWelcome();
void showHelpText();
void showInfo();
void dirRow(const String& name, const String& tag);
void showRootDir();
void showOptDir();
void showLabDir();
void showSdDir();
void drawMenuScrollbar(int total, int visible, int selected, int y, int h);
String optBar(int v);
String passText();
void showOptions();
void optUp();
void optDown();
void optLeft();
void optRight();
void startConfigEdit(EditKind kind);
void renderConfigEdit();
void saveConfigEdit();
void showLabScreen();
void showBleScreen();
void startBallApp();
void updateBallApp();

// WLAN
String wlanRowText(const WifiEntry& e);
void showWlanList(bool doScan);
WifiEntry selectedWifi();
int wlanInfoTotalRows();
String wlanInfoLine(int row);
void showWlanInfo();
void startWlanPassword();
void renderWlanPassword();
void connectSelectedWlan();

// filesystem
void sdGoBack();
String pathJoin(const String& base, const String& name);
bool entryIsDir(const String& path, bool& isDir);
bool removeRecursive(const String& path);
bool copyFileData(const String& src, const String& dst);
bool copyDirRecursive(const String& src, const String& dst);
String conflictFreePath(const String& dir, const String& name, bool isDir);
void newFile(String name);
void delEntry(String name);
void copyEntry(String name);
void pasteEntry();
void catFile(String name);
void mkDir(String name);
void initLayout();

// audio
String msToClock(uint32_t ms);
void audioWaveReset();
void audioWavePush(int16_t level);
void drawAudioWave();
void drawAudioRecorderStatic(const String& title);
void drawAudioRecorderDynamic();
void showAudioRecorderScreen();
void showAudioSaveMenu();
void renderAudioNameEdit();
void startAudioNameEdit();
void saveAudioWithName();
void openAudioPlayerByPath(const String& path, const String& name);
void showAudioPlayerScreen();
void drawAudioPlayerDynamic();
void audioUiTick();

// editor
int editorVisibleRows();
String lineNumber(int index);
int editorTextRows();
int editorVisibleCols();
int editorMaxScrollY();
int editorMaxScrollX();
void clampEditorCursor();
void clampEditorScroll();
void editorFollowCursor();
void renderEditorExitMenu();
void renderEditor();
bool saveEditorFile();
void editorExitNoSave();
void editorNewLine();
void editorBackspace();
void editorAddChar(char c);
void editorMoveUp();
void editorMoveDown();
void editorMoveLeft();
void editorMoveRight();
void editorOpenExitMenu();

void drawEditorCursorOnly();
void openTextEditor(String name);

// command/runtime
void showVersion();
void showAbout();
void executeCommand(const String& raw);
