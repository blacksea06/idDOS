#include "shell_internal.h"

// AUDIO LAB / RECORDER / PLAYER
// ───────────────────────────────────────────────────────────────────

String msToClock(uint32_t ms) {
    uint32_t sec = ms / 1000UL;
    uint32_t m = sec / 60UL;
    uint32_t s = sec % 60UL;
    String out;
    if (m < 10) out += '0';
    out += String(m);
    out += ':';
    if (s < 10) out += '0';
    out += String(s);
    return out;
}

static int audioWaveSmooth = 0;

void audioWaveReset() {
    for (int i = 0; i < 120; ++i) audioWave[i] = 0;
    audioWavePos = 0;
    audioLastWaveMs = 0;
    audioWaveSmooth = 0;
}

void audioWavePush(int16_t level) {
    // Sensitive ECG-style response: silence stays calm, voice becomes tall
    // narrow spikes instead of the old small alternating jumper.
    int raw = abs((int)level);
    int gated = raw - 180;
    if (gated < 0) gated = 0;

    int amp = map(gated, 0, 7200, 0, 37);
    if (amp < 0) amp = 0;
    if (amp > 37) amp = 37;

    // Fast attack, slower release. This keeps the wave alive without noise.
    if (amp > audioWaveSmooth) audioWaveSmooth = (audioWaveSmooth + amp * 3) / 4;
    else audioWaveSmooth = (audioWaveSmooth * 5 + amp) / 6;

    int a = audioWaveSmooth;
    int phase = audioWavePos % 7;
    int v = 0;

    if (a > 1) {
        // QRS-like silhouette: baseline, dip, spike, return.
        if      (phase == 1) v = -a / 3;
        else if (phase == 2) v =  a;
        else if (phase == 3) v = -a / 2;
        else if (phase == 4) v =  a / 4;
        else                 v =  0;
    }

    audioWave[audioWavePos] = v;
    audioWavePos = (audioWavePos + 1) % 120;
}

void drawAudioWave() {
    const int x0 = 5;
    const int y0 = 45;
    const int w = 230;
    const int h = 51;
    const int cy = y0 + h / 2;

    // Only the living ECG wave. No top/bottom guide lines, no grid.
    M5Cardputer.Display.fillRect(x0, y0, w, h, BLACK);

    int lastX = x0;
    int lastY = cy;
    for (int i = 0; i < 120; ++i) {
        int idx = (audioWavePos + i) % 120;
        int x = x0 + (i * w) / 119;
        int y = cy - audioWave[idx];
        if (y < y0 + 1) y = y0 + 1;
        if (y > y0 + h - 2) y = y0 + h - 2;
        if (i > 0) M5Cardputer.Display.drawLine(lastX, lastY, x, y, GREEN);
        lastX = x;
        lastY = y;
    }
}

void drawAudioRecorderStatic(const String& title) {
    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();
    printAt(5, Dim::HIST_Y0, title, CYAN);
}

void drawAudioRecorderDynamic() {
    M5Cardputer.Display.fillRect(5, 31, 230, 12, BLACK);

    if (AudioLab::recordingActive()) {
        bool blink = ((millis() / 500) & 1) == 0;
        if (blink) M5Cardputer.Display.fillCircle(11, 36, 4, RED);
        printAt(20, 32, "REC " + msToClock(AudioLab::recordMillis()), RED);
        printAt(90, 32, String(AudioLab::recordBytes() / 1024) + "KB", DARKGREY);
        printAt(5, Dim::PROMPT_Y, "ENTER stop  ESC discard", WHITE);
    } else {
        printAt(5, 32, AudioLab::available() ? "LIVE MIC" : "AUDIO INIT FAIL", AudioLab::available() ? WHITE : RED);
        printAt(5, Dim::PROMPT_Y, "ENTER rec  ESC lab", WHITE);
    }

    drawAudioWave();
}

void showAudioRecorderScreen() {
    S.uiMode = UI_AUDIO_RECORDER;
    S.section = LAB_SECTION;
    S.audioLastDrawMs = 0;
    audioWaveReset();

    AudioLab::begin();
    drawAudioRecorderStatic("AUDIO RECORDER");
    drawAudioRecorderDynamic();
}

void showAudioSaveMenu() {
    S.uiMode = UI_AUDIO_SAVE_MENU;
    S.section = LAB_SECTION;
    if (S.audioSaveSel < 0) S.audioSaveSel = 0;
    if (S.audioSaveSel > 1) S.audioSaveSel = 1;

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "SAVE RECORD?", CYAN);

    bool saveSel = S.audioSaveSel == 0;
    bool discardSel = S.audioSaveSel == 1;

    printAt(5, 48, saveSel ? ">" : " ", GREEN);
    printAt(18, 48, "SAVE", saveSel ? GREEN : WHITE);

    printAt(5, 66, discardSel ? ">" : " ", GREEN);
    printAt(18, 66, "DISCARD", discardSel ? GREEN : WHITE);

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER choose  ESC discard", WHITE);
}

void renderAudioNameEdit() {
    M5Cardputer.Display.fillRect(0, 43, Dim::SCREEN_W, 36, BLACK);
    printAt(5, 48, ">", GREEN);
    printAt(18, 48, trimToWidth(S.audioNameEdit + "_", 34), WHITE);
    printAt(5, 66, ".wav will be added", DARKGREY);
}

void startAudioNameEdit() {
    S.uiMode = UI_AUDIO_NAME_EDIT;
    S.section = LAB_SECTION;
    S.audioNameEdit = "rec" + String(millis() / 1000UL);

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();
    printAt(5, Dim::HIST_Y0, "RECORD NAME", CYAN);
    renderAudioNameEdit();
    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER save  ESC cancel", WHITE);
}

void saveAudioWithName() {
    String out;
    if (AudioLab::saveRecordAs(S.audioNameEdit, out)) {
        S.uiMode = UI_SHELL;
        S.section = SD_SECTION;
        S.sdPath = "/audio";
        addBreadcrumb();
        addHistory("AUDIO SAVED", GREEN);
        addHistory(out, WHITE);
        soundSave();
        AudioLab::idleSilence();
        renderAll();
    } else {
        soundError();
        printAt(5, 83, "SAVE FAIL", RED);
    }
}

void openAudioPlayerByPath(const String& path, const String& name) {
    if (!AudioLab::playerOpen(path)) {
        addHistory("AUDIO OPEN FAIL", RED);
        soundError();
        return;
    }

    S.uiMode = UI_AUDIO_PLAYER;
    S.audioPlayerPath = path;
    S.audioPlayerName = name;
    showAudioPlayerScreen();
}

static void drawAudioPlayerFrame()
{
    M5Cardputer.Display.drawRect(4, 54, 232, 55, DARKGREY);
    M5Cardputer.Display.drawFastHLine(12, 78, 216, DARKGREY);
    M5Cardputer.Display.drawFastHLine(12, 79, 216, DARKGREY);
}

void drawAudioPlayerDynamic()
{
    // Update only small moving fields. Do not clear/redraw the whole deck,
    // otherwise the Cardputer LCD shows a visible black flash/flicker.
    bool playing = AudioLab::playerPlaying();

    M5Cardputer.Display.fillRect(8, 58, 220, 10, BLACK);
    printAt(10, 59, playing ? "[ PAUSE ]" : "[ PLAY  ]", playing ? YELLOW : GREEN);

    uint32_t pos = AudioLab::playerPositionMs();
    uint32_t dur = AudioLab::playerDurationMs();
    printAt(92, 59, msToClock(pos) + "/" + msToClock(dur), DARKGREY);

    const int barX = 12;
    const int barY = 78;
    const int barW = 216;
    M5Cardputer.Display.fillRect(barX, barY - 4, barW, 11, BLACK);
    M5Cardputer.Display.drawFastHLine(barX, barY, barW, DARKGREY);
    M5Cardputer.Display.drawFastHLine(barX, barY + 1, barW, DARKGREY);

    uint32_t fill = 0;
    if (dur > 0) fill = (uint32_t)((uint64_t)barW * pos / dur);
    if (fill > (uint32_t)barW) fill = barW;

    if (fill > 0) M5Cardputer.Display.drawFastHLine(barX, barY, (int)fill, GREEN);

    int marker = barX + (int)fill;
    if (marker < barX) marker = barX;
    if (marker > barX + barW - 1) marker = barX + barW - 1;
    M5Cardputer.Display.fillRect(marker - 1, barY - 3, 3, 8, GREEN);

    int pct = (int)((uint32_t)AudioLab::playerVolume() * 100UL / 255UL);
    String vol = "VOL ";
    int bars = map(pct, 0, 100, 0, 10);
    vol += "[";
    for (int i = 0; i < 10; ++i) vol += (i < bars) ? '|' : ' ';
    vol += "] ";
    vol += String(pct);
    vol += "%";
    M5Cardputer.Display.fillRect(8, 92, 220, 10, BLACK);
    printAt(10, 93, vol, CYAN);
}

void showAudioPlayerScreen() {
    S.uiMode = UI_AUDIO_PLAYER;
    S.audioLastDrawMs = 0;

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "WAV DECK", CYAN);
    printAt(5, 37, trimToWidth(S.audioPlayerName.length() ? S.audioPlayerName : AudioLab::playerName(), 34), WHITE);

    drawAudioPlayerFrame();
    drawAudioPlayerDynamic();

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER play/pause  Aa +/- vol  ESC", WHITE);
}

void audioUiTick() {
    if (S.uiMode == UI_AUDIO_RECORDER) {
        if (millis() - audioLastWaveMs < 40) return;
        audioLastWaveMs = millis();

        int16_t lvl = 0;
        if (AudioLab::recordingActive()) {
            AudioLab::updateRecord();
            lvl = AudioLab::lastLevel();
        } else {
            lvl = AudioLab::readLiveLevel();
        }

        audioWavePush(lvl);
        drawAudioRecorderDynamic();
        return;
    }

    if (S.uiMode == UI_AUDIO_PLAYER) {
        AudioLab::playerUpdate();
        if (millis() - S.audioLastDrawMs > 500) {
            S.audioLastDrawMs = millis();
            drawAudioPlayerDynamic();
        }
    }
}

// ───────────────────────────────────────────────────────────────────
