#include "shell_internal.h"
#include "keys_compat.h"
#include <math.h>
#include <string.h>

void showBleScreen()
{
    S.uiMode = UI_BLE_INFO;
    S.section = LAB_SECTION;

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "BLE GATT SERVER", CYAN);

    bool serverOn = bleServerRunning();
    bool advOn = bleAdvertisingRunning();

   bool sel0 = (S.bleMenuSel == 0);
bool sel1 = (S.bleMenuSel == 1);

printAt(5, 44, sel0 ? ">" : " ", GREEN);
printAt(18, 44,
        (serverOn || advOn) ? "STOP BLE" : "START BLE",
        sel0 ? GREEN : WHITE);

printAt(5, 62, sel1 ? ">" : " ", GREEN);
printAt(18, 62,
        "NAME   " + bleDeviceName(),
        sel1 ? GREEN : WHITE);

printAt(18, 80, "STATUS " + bleStatusText(), WHITE);

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER toggle  ESC lab", WHITE);
}

// BALL / BREAKOUT LAB GAME
// Green idDOS-style Breakout game. Core is not touched: this is a local LAB app
// replacement for the old bouncing ball demo.
// Flicker fix: no full-frame repaint in updateBallApp(). Static bricks are drawn
// once, then only dirty rectangles for the ball/paddle/HUD/message are updated.
// Controls:
//   - tilt left/right: move paddle
//   - tilt forward: push paddle up for an active hit
//   - ENTER/SPACE: launch or restart
//   - FN + ESC: exit to LAB
// -------------------------------------------------------------------
namespace {
constexpr int GAME_W = Dim::SCREEN_W;
constexpr int GAME_H = Dim::SCREEN_H;
constexpr int GAME_TOP = Dim::STATUS_H + 1;
constexpr int HUD_Y = 0;
constexpr int HUD_H = Dim::STATUS_H;
constexpr int BALL_R = 3;
constexpr int BALL_ERASE_PAD = BALL_R + 2;
constexpr int PADDLE_W = 42;
constexpr int PADDLE_H = 5;
constexpr int PADDLE_BASE_Y = 124;
constexpr int PADDLE_PUSH_MAX = 37;
constexpr int BRICK_ROWS = 4;
constexpr int BRICK_COLS = 8;
constexpr int BRICK_W = 26;
constexpr int BRICK_H = 7;
constexpr int BRICK_GAP = 2;
constexpr int BRICK_X0 = 7;
constexpr int BRICK_Y0 = 24;
constexpr int MAX_LIVES = 3;
constexpr int MSG_Y = 75;
constexpr int MSG_H = 10;
constexpr float BALL_SPEED_START = 2.35f;
constexpr float BALL_SPEED_MAX = 4.6f;

float ballX = GAME_W / 2.0f;
float ballY = PADDLE_BASE_Y - BALL_R - 1.0f;
float oldBallX = ballX;
float oldBallY = ballY;
float ballVx = 0.0f;
float ballVy = 0.0f;
float paddleX = (GAME_W - PADDLE_W) / 2.0f;
float paddleY = PADDLE_BASE_Y;
float oldPaddleX = paddleX;
float oldPaddleY = paddleY;
int score = 0;
int lastScore = -1;
int lives = MAX_LIVES;
int lastLives = -1;
int bricksLeft = BRICK_ROWS * BRICK_COLS;
bool bricks[BRICK_ROWS][BRICK_COLS];
bool ballLaunched = false;
bool gameOver = false;
bool gameWon = false;
bool staticFrameDirty = true;
const char* lastMessage = nullptr;
uint32_t ballLastFrameMs = 0;

void eraseBallAt(float x, float y)
{
    auto &d = M5Cardputer.Display;
    d.fillRect((int)x - BALL_ERASE_PAD, (int)y - BALL_ERASE_PAD,
               BALL_ERASE_PAD * 2 + 1, BALL_ERASE_PAD * 2 + 1, BLACK);
}

void erasePaddleAt(float x, float y)
{
    auto &d = M5Cardputer.Display;
    d.fillRect((int)x - 2, (int)y - 2, PADDLE_W + 4, PADDLE_H + 4, BLACK);
}

uint16_t brickColor(int row)
{
    // System style: green shades only. Top rows brighter = higher value target.
    switch (row) {
        case 0: return GREEN;
        case 1: return 0x05E0; // medium green
        case 2: return 0x03E0; // darker green
        default: return 0x0240; // dim matrix green
    }
}

void drawBrickCell(int r, int c)
{
    auto &d = M5Cardputer.Display;
    const int x = BRICK_X0 + c * (BRICK_W + BRICK_GAP);
    const int y = BRICK_Y0 + r * (BRICK_H + BRICK_GAP);

    if (!bricks[r][c]) {
        d.fillRect(x, y, BRICK_W, BRICK_H, BLACK);
        return;
    }

    d.fillRect(x, y, BRICK_W, BRICK_H, brickColor(r));
    d.drawRect(x, y, BRICK_W, BRICK_H, BLACK);
    d.drawFastHLine(x + 2, y + 1, BRICK_W - 4, 0x07E0);
}

void drawBricks()
{
    for (int r = 0; r < BRICK_ROWS; ++r) {
        for (int c = 0; c < BRICK_COLS; ++c) drawBrickCell(r, c);
    }
}

void resetBricks()
{
    bricksLeft = BRICK_ROWS * BRICK_COLS;
    for (int r = 0; r < BRICK_ROWS; ++r) {
        for (int c = 0; c < BRICK_COLS; ++c) bricks[r][c] = true;
    }
}

void clampBallSpeed()
{
    const float speed = sqrtf(ballVx * ballVx + ballVy * ballVy);
    if (speed < 0.01f) return;
    if (speed > BALL_SPEED_MAX) {
        const float k = BALL_SPEED_MAX / speed;
        ballVx *= k;
        ballVy *= k;
    }
}

void resetBallOnPaddle()
{
    ballLaunched = false;
    gameOver = false;
    gameWon = false;
    ballVx = 0.0f;
    ballVy = 0.0f;
    ballX = paddleX + PADDLE_W * 0.5f;
    ballY = paddleY - BALL_R - 1;
    oldBallX = ballX;
    oldBallY = ballY;
}

void launchBall()
{
    if (gameOver || gameWon) {
        score = 0;
        lives = MAX_LIVES;
        resetBricks();
        gameOver = false;
        gameWon = false;
        staticFrameDirty = true;
    }

    ballLaunched = true;
    float rel = ((ballX - paddleX) / PADDLE_W) - 0.5f;
    ballVx = rel * 2.4f;
    ballVy = -BALL_SPEED_START;
    if (fabsf(ballVx) < 0.45f) ballVx = 0.55f;
}

void drawBallHud(bool force = false)
{
    if (!force && score == lastScore && lives == lastLives) return;
    lastScore = score;
    lastLives = lives;

    auto &d = M5Cardputer.Display;
    d.fillRect(0, HUD_Y, GAME_W, HUD_H, BLACK);
    d.setTextSize(1);

    d.setTextColor(GREEN, BLACK);
    d.setCursor(3, 4);
    d.print("BALL");

    d.setTextColor(GREEN, BLACK);
    d.setCursor(42, 4);
    d.print("PTS ");
    d.print(score);

    d.setCursor(101, 4);
    d.print("L ");
    d.print(lives);

    d.setTextColor(DARKGREY, BLACK);
    d.setCursor(148, 4);
    d.print("FN+ESC EXIT");
}

void drawStaticFrame()
{
    auto &d = M5Cardputer.Display;
    d.fillScreen(BLACK);
    drawBallHud(true);
    d.drawFastHLine(0, GAME_TOP, GAME_W, GREEN);
    drawBricks();
    staticFrameDirty = false;
    lastMessage = nullptr;
}

void drawMessage(const char* message)
{
    if (message == lastMessage) return;
    lastMessage = message;

    auto &d = M5Cardputer.Display;
    d.fillRect(0, MSG_Y, GAME_W, MSG_H, BLACK);
    if (!message) return;

    d.setTextSize(1);
    d.setTextColor(GREEN, BLACK);
    int x = 12;
    const int len = (int)strlen(message);
    if (len < 30) x = (GAME_W - len * Dim::CHAR_W) / 2;
    d.setCursor(x, MSG_Y);
    d.print(message);
}

void drawMovingObjects()
{
    auto &d = M5Cardputer.Display;

    eraseBallAt(oldBallX, oldBallY);
    erasePaddleAt(oldPaddleX, oldPaddleY);

    d.fillRoundRect((int)paddleX, (int)paddleY, PADDLE_W, PADDLE_H, 2, GREEN);
    d.drawFastHLine((int)paddleX + 4, (int)paddleY + 1, PADDLE_W - 8, WHITE);
    d.fillCircle((int)ballX, (int)ballY, BALL_R, GREEN);
    d.drawPixel((int)ballX - 1, (int)ballY - 1, WHITE);

    oldBallX = ballX;
    oldBallY = ballY;
    oldPaddleX = paddleX;
    oldPaddleY = paddleY;
}

void handleBrickHit()
{
    for (int r = 0; r < BRICK_ROWS; ++r) {
        for (int c = 0; c < BRICK_COLS; ++c) {
            if (!bricks[r][c]) continue;

            const int bx = BRICK_X0 + c * (BRICK_W + BRICK_GAP);
            const int by = BRICK_Y0 + r * (BRICK_H + BRICK_GAP);
            const bool hitX = ballX + BALL_R >= bx && ballX - BALL_R <= bx + BRICK_W;
            const bool hitY = ballY + BALL_R >= by && ballY - BALL_R <= by + BRICK_H;
            if (!hitX || !hitY) continue;

            bricks[r][c] = false;
            bricksLeft--;
            score += (BRICK_ROWS - r) * 10;
            drawBrickCell(r, c);

            const float overlapLeft = fabsf((ballX + BALL_R) - bx);
            const float overlapRight = fabsf((bx + BRICK_W) - (ballX - BALL_R));
            const float overlapTop = fabsf((ballY + BALL_R) - by);
            const float overlapBottom = fabsf((by + BRICK_H) - (ballY - BALL_R));
            const float minX = fminf(overlapLeft, overlapRight);
            const float minY = fminf(overlapTop, overlapBottom);

            if (minX < minY) ballVx = -ballVx;
            else ballVy = -ballVy;

            if (ballVy > -0.35f && ballVy < 0.35f) ballVy = (ballVy < 0.0f) ? -0.9f : 0.9f;
            clampBallSpeed();

            if (bricksLeft <= 0) {
                gameWon = true;
                ballLaunched = false;
            }
            return;
        }
    }
}

void updatePaddleFromImu()
{
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    M5.Imu.getAccel(&ax, &ay, &az);

    paddleX += -ax * 5.8f;
    if (paddleX < 0) paddleX = 0;
    if (paddleX > GAME_W - PADDLE_W) paddleX = GAME_W - PADDLE_W;

    float push = -ay * PADDLE_PUSH_MAX;
    if (push < 0) push = 0;
    if (push > PADDLE_PUSH_MAX) push = PADDLE_PUSH_MAX;
    paddleY = PADDLE_BASE_Y - push;

    if (!ballLaunched) {
        ballX = paddleX + PADDLE_W * 0.5f;
        ballY = paddleY - BALL_R - 1;
    }
}

void updateBallPhysics()
{
    if (!ballLaunched || gameOver || gameWon) return;

    ballX += ballVx;
    ballY += ballVy;

    if (ballX < BALL_R) { ballX = BALL_R; ballVx = fabsf(ballVx); }
    if (ballX > GAME_W - BALL_R) { ballX = GAME_W - BALL_R; ballVx = -fabsf(ballVx); }
    if (ballY < GAME_TOP + BALL_R) { ballY = GAME_TOP + BALL_R; ballVy = fabsf(ballVy); }

    handleBrickHit();

    const bool hitPaddleX = ballX + BALL_R >= paddleX && ballX - BALL_R <= paddleX + PADDLE_W;
    const bool hitPaddleY = ballY + BALL_R >= paddleY && ballY - BALL_R <= paddleY + PADDLE_H;
    if (ballVy > 0 && hitPaddleX && hitPaddleY) {
        ballY = paddleY - BALL_R - 1;
        float rel = ((ballX - paddleX) / PADDLE_W) - 0.5f;
        ballVx += rel * 1.8f;
        ballVy = -fabsf(ballVy) - 0.08f;
        clampBallSpeed();
    }

    if (ballY > GAME_H + BALL_R) {
        lives--;
        if (lives <= 0) {
            lives = 0;
            gameOver = true;
            ballLaunched = false;
        } else {
            resetBallOnPaddle();
        }
    }
}
}

void startBallApp()
{
    S.uiMode = UI_BALL_APP;
    S.section = LAB_SECTION;
    score = 0;
    lastScore = -1;
    lives = MAX_LIVES;
    lastLives = -1;
    paddleX = (GAME_W - PADDLE_W) / 2.0f;
    paddleY = PADDLE_BASE_Y;
    oldPaddleX = paddleX;
    oldPaddleY = paddleY;
    resetBricks();
    resetBallOnPaddle();
    ballLastFrameMs = 0;
    staticFrameDirty = true;

    drawStaticFrame();
    drawMessage("ENTER/SPACE TO START");
    drawMovingObjects();
}

void updateBallApp()
{
    auto ks = M5Cardputer.Keyboard.keysState();
    const bool changedPressed = M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed();
    if (changedPressed && ks.fn && keyEsc(ks)) {
        S.uiMode = UI_LAB;
        showLabScreen();
        return;
    }

    if (changedPressed && (ks.enter || ks.space)) launchBall();

    const uint32_t now = millis();
    if (now - ballLastFrameMs < 16) return;
    ballLastFrameMs = now;

    if (staticFrameDirty) drawStaticFrame();

    updatePaddleFromImu();
    updateBallPhysics();
    drawBallHud();

    if (gameWon) drawMessage("YOU WIN - ENTER RESTART");
    else if (gameOver) drawMessage("GAME OVER - ENTER RESTART");
    else if (!ballLaunched) drawMessage("ENTER/SPACE TO START");
    else drawMessage(nullptr);

    drawMovingObjects();
}
void shellUpdate() {
    if (millis() - S.batLastMs > 30000) {
        S.batLastMs = millis();
        drawStatusBar();
    }

    audioUiTick();

    static uint32_t lastCmdBlinkMs = 0;
    static uint32_t lastEditorBlinkMs = 0;

    if (!editorMode && S.uiMode == UI_SHELL && millis() - lastCmdBlinkMs >= 450) {
        lastCmdBlinkMs = millis();
        drawCmdCursorOnly();
    }

    if (editorMode && !editorSaveMode && millis() - lastEditorBlinkMs >= 450) {
        lastEditorBlinkMs = millis();
        drawEditorCursorOnly();
    }

    if (S.uiMode == UI_HW_TOOLS) {
        HardwareTools::update();
        if (HardwareTools::exitRequested()) {
            S.uiMode = UI_LAB;
            showLabScreen();
        }
        return;
    }

    if (S.uiMode == UI_BALL_APP) {
        updateBallApp();
        return;
    }

    if (!M5Cardputer.Keyboard.isChange() ||
        !M5Cardputer.Keyboard.isPressed()) {
        return;
    }

    auto ks = M5Cardputer.Keyboard.keysState();

    // TEXT EDITOR MODE
    if (editorMode) {
        if (editorSaveMode) {
            if (keyEsc(ks)) {
                editorSaveMode = false;
                renderEditor();
                return;
            }

            if (keyUp(ks) || (ks.fn && keyUp(ks))) {
                if (editorSaveSelection > 0) editorSaveSelection--;
                renderEditorExitMenu();
                return;
            }

            if (keyDown(ks) || (ks.fn && keyDown(ks))) {
                if (editorSaveSelection < 2) editorSaveSelection++;
                renderEditorExitMenu();
                return;
            }

            if (ks.enter) {
                if (editorSaveSelection == 0) {
                    bool ok = saveEditorFile();
                    editorExitNoSave();
                    addHistory(ok ? "SAVED" : "SAVE FAIL", ok ? GREEN : RED);
                    if (ok) soundSave();
                    else soundError();
                    renderAll();
                    return;
                }

                if (editorSaveSelection == 1) {
                    editorExitNoSave();
                    addHistory("CLOSED NO SAVE", YELLOW);
                    renderAll();
                    return;
                }

                editorSaveMode = false;
                renderEditor();
                return;
            }

            return;
        }

        if (ks.fn && keyEsc(ks)) {
            editorOpenExitMenu();
            return;
        }

        if (ks.enter) {
            editorNewLine();
            return;
        }

        if (keyBackspace(ks)) {
            editorBackspace();
            return;
        }

        if (keyUp(ks) || (ks.fn && keyUp(ks))) {
            editorMoveUp();
            return;
        }

        if (keyDown(ks) || (ks.fn && keyDown(ks))) {
            editorMoveDown();
            return;
        }

        if (keyLeft(ks) || (ks.fn && keyLeft(ks))) {
            editorMoveLeft();
            return;
        }

        if (keyRight(ks) || (ks.fn && keyRight(ks))) {
            editorMoveRight();
            return;
        }

        for (auto c : ks.word) {
            if (c >= 32 && c <= 126) editorAddChar(c);
        }

        return;
    }

    // AUDIO RECORDER MODE
    if (S.uiMode == UI_AUDIO_RECORDER) {
        if (keyEsc(ks) || (ks.fn && keyEsc(ks))) {
            if (AudioLab::recordingActive()) {
                AudioLab::discardRecord();
                AudioLab::idleSilence();
            }
            showLabScreen();
            return;
        }

        if (ks.enter) {
            if (AudioLab::recordingActive()) {
                if (AudioLab::stopRecord()) {
                    AudioLab::idleSilence();
                    S.audioSaveSel = 0;
                    showAudioSaveMenu();
                } else {
                    soundError();
                    drawAudioRecorderDynamic();
                }
                return;
            }

            if (!AudioLab::startRecord()) {
                soundError();
            }
            drawAudioRecorderDynamic();
            return;
        }

        return;
    }

    // AUDIO SAVE MENU MODE
    if (S.uiMode == UI_AUDIO_SAVE_MENU) {
        if (keyEsc(ks) || (ks.fn && keyEsc(ks))) {
            AudioLab::discardRecord();
            AudioLab::idleSilence();
            showAudioRecorderScreen();
            return;
        }

        if (keyUp(ks) || keyDown(ks) || (ks.fn && keyUp(ks)) || (ks.fn && keyDown(ks))) {
            S.audioSaveSel = 1 - S.audioSaveSel;
            showAudioSaveMenu();
            return;
        }

        if (ks.enter) {
            if (S.audioSaveSel == 0) {
                startAudioNameEdit();
            } else {
                AudioLab::discardRecord();
                AudioLab::idleSilence();
                showAudioRecorderScreen();
            }
            return;
        }

        return;
    }

    // AUDIO NAME EDIT MODE
    if (S.uiMode == UI_AUDIO_NAME_EDIT) {
        if (keyEsc(ks) || (ks.fn && keyEsc(ks))) {
            showAudioSaveMenu();
            return;
        }

        if (ks.enter) {
            saveAudioWithName();
            return;
        }

        if (keyBackspace(ks)) {
            if (S.audioNameEdit.length() > 0) {
                S.audioNameEdit.remove(S.audioNameEdit.length() - 1);
                renderAudioNameEdit();
            }
            return;
        }

        for (auto c : ks.word) {
            if (c >= 32 && c <= 126 && S.audioNameEdit.length() < 24) {
                if (c == ' ') c = '_';
                S.audioNameEdit += c;
                renderAudioNameEdit();
            }
        }
        return;
    }

    // AUDIO PLAYER MODE
    if (S.uiMode == UI_AUDIO_PLAYER) {
        if (keyEsc(ks) || (ks.fn && keyEsc(ks))) {
            AudioLab::playerClose();
            S.uiMode = UI_SHELL;
            renderAll();
            return;
        }

        bool volUp = false;
        bool volDown = false;
        for (auto c : ks.word) {
            if (c == '+' || c == '=') volUp = true;
            if (c == '-' || c == '_') volDown = true;
        }

        if (volUp || keyRight(ks) || (ks.fn && keyRight(ks))) {
            uint8_t v = AudioLab::playerVolume();
            v = (v > 240) ? 255 : (uint8_t)(v + 15);
            AudioLab::playerSetVolume(v);
            drawAudioPlayerDynamic();
            return;
        }

        if (volDown || keyLeft(ks) || (ks.fn && keyLeft(ks))) {
            uint8_t v = AudioLab::playerVolume();
            v = (v < 15) ? 0 : (uint8_t)(v - 15);
            AudioLab::playerSetVolume(v);
            drawAudioPlayerDynamic();
            return;
        }

        if (ks.enter) {
            if (AudioLab::playerPlaying()) {
                AudioLab::playerStop();
            } else {
                AudioLab::playerStart();
            }
            drawAudioPlayerDynamic();
            return;
        }

        return;
    }

    // CONFIG EDIT MODE
    if (S.uiMode == UI_CONFIG_EDIT) {
        if (keyEsc(ks)) {
            S.editKind = EDIT_NONE;
            S.editBuffer = "";
            showOptions();
            return;
        }

        if (ks.enter) {
            saveConfigEdit();
            return;
        }

        if (keyBackspace(ks)) {
            if (S.editBuffer.length() > 0) {
                S.editBuffer.remove(S.editBuffer.length() - 1);
                renderConfigEdit();
            }
            return;
        }

        for (auto c : ks.word) {
            if (c >= 32 && c <= 126 && S.editBuffer.length() < (S.editKind == EDIT_PASSWORD ? Limit::PASSWORD_MAX : 32)) {
                S.editBuffer += c;
                renderConfigEdit();
            }
        }

        return;
    }



    // OPTIONS MODE
    if (S.uiMode == UI_OPTIONS) {
        if (keyEsc(ks)) {
            S.uiMode = UI_SHELL;
            S.section = ROOT;
            renderAll();
            return;
        }

        if (keyUp(ks) || (ks.fn && keyUp(ks))) {
            optUp();
            return;
        }

        if (keyDown(ks) || (ks.fn && keyDown(ks))) {
            optDown();
            return;
        }

        if (keyLeft(ks)) {
            optLeft();
            return;
        }

        if (keyRight(ks)) {
            optRight();
            return;
        }

        if (ks.enter) {
            if (S.optSel == 0) {
                startConfigEdit(EDIT_USERNAME);
                return;
            }
            if (S.optSel == 1) {
                startConfigEdit(EDIT_PASSWORD);
                return;
            }
            if (S.optSel == 2) {
                soundSetEnabled(!soundIsEnabled());
                soundSuccess();
                showOptions();
                return;
            }
        }

        return;
    }

  // LAB MODE
if (S.uiMode == UI_LAB) {
    if (keyEsc(ks)) {
        S.uiMode = UI_SHELL;
        S.section = ROOT;
        renderAll();
        return;
    }

    if (keyUp(ks) || (ks.fn && keyUp(ks))) {
        if (S.labSel > 0) S.labSel--;
        showLabScreen();
        return;
    }

    if (keyDown(ks) || (ks.fn && keyDown(ks))) {
        if (S.labSel < Limit::LAB_TOTAL - 1) S.labSel++;
        showLabScreen();
        return;
    }

    if (ks.enter) {
      if (S.labSel == 0) {
    bleStopForWifi();
    delay(500);
    showWlanList(true);
    return;
}

        if (S.labSel == 1) {
            showBleScreen();
            return;
        }
    if (S.labSel == 2) {

        startBallApp();

        return;

    }

    if (S.labSel == 3) {
        showAudioRecorderScreen();
        return;
    }

    if (S.labSel == 4) {
        HardwareTools::openMenu();
        S.uiMode = UI_HW_TOOLS;
        return;
    }
    }

    return;
}

    // WLAN LIST MODE
    if (S.uiMode == UI_WLAN_LIST) {
        if (keyEsc(ks)) {
            showLabScreen();
            return;
        }

        if (ks.fn && ks.enter) {
    bleStopForWifi();
    delay(500);
    showWlanList(true);
    return;
}

        if (keyUp(ks) || (ks.fn && keyUp(ks))) {
            if (S.wlanSel > 0) S.wlanSel--;
            showWlanList(false);
            return;
        }

        if (keyDown(ks) || (ks.fn && keyDown(ks))) {
            if (S.wlanSel < S.wlanCount - 1) S.wlanSel++;
            showWlanList(false);
            return;
        }

        if (ks.enter) {
            if (S.wlanCount <= 0) {
                showWlanList(true);
                return;
            }

            S.selectedNet = S.wlanSel;
            S.wlanInfoScroll = 0;
            showWlanInfo();
            return;
        }

        return;
    }

    // WLAN INFO MODE
    if (S.uiMode == UI_WLAN_INFO) {
        if (keyEsc(ks)) {
            showWlanList(false);
            return;
        }

        if (keyUp(ks) || (ks.fn && keyUp(ks))) {
            if (S.wlanInfoScroll > 0) S.wlanInfoScroll--;
            showWlanInfo();
            return;
        }

        if (keyDown(ks) || (ks.fn && keyDown(ks))) {
            int maxScroll = wlanInfoTotalRows() - Limit::WLAN_INFO_VISIBLE;
            if (maxScroll < 0) maxScroll = 0;
            if (S.wlanInfoScroll < maxScroll) S.wlanInfoScroll++;
            showWlanInfo();
            return;
        }

        if (ks.enter) {
            WifiEntry e = selectedWifi();
            bool connected = networkConnected() && e.ssid.length() && (networkCurrentSSID() == e.ssid);

if (connected) {
    networkDisconnect();
    soundSuccess();
    delay(300);
    showWlanInfo();
    return;
}

            if (e.security == "OPEN") {
                S.wlanPassword = "";
                connectSelectedWlan();
                return;
            }

            startWlanPassword();
            return;
        }

        return;
    }

        // WLAN PASSWORD MODE
    if (S.uiMode == UI_WLAN_PASS) {
        if (keyEsc(ks)) {
            S.wlanPassword = "";
            showWlanInfo();
            return;
        }

        if (ks.enter) {
            connectSelectedWlan();
            return;
        }

        if (keyBackspace(ks)) {
            if (S.wlanPassword.length() > 0) {
                S.wlanPassword.remove(S.wlanPassword.length() - 1);
                renderWlanPassword();
            }
            return;
        }

        for (auto c : ks.word) {
            if (c >= 32 && c <= 126 && S.wlanPassword.length() < Limit::PASSWORD_MAX) {
                S.wlanPassword += c;
                renderWlanPassword();
            }
        }

        return;
    }

    // BLE NAME EDIT MODE
    if (S.uiMode == UI_BLE_NAME_EDIT) {
    if (keyEsc(ks)) {
        S.bleNameEdit = "";
        S.uiMode = UI_BLE_INFO;
        showBleScreen();
        return;
    }

    if (ks.enter) {
        if (bleAdvertisingRunning() || bleServerRunning()) {
            soundError();
            S.bleNameEdit = "";
            S.uiMode = UI_BLE_INFO;
            showBleScreen();
            return;
        }

        bleSetDeviceName(S.bleNameEdit);

        S.bleNameEdit = "";
        S.uiMode = UI_BLE_INFO;
        showBleScreen();
        return;
    }

    if (keyBackspace(ks)) {
        if (S.bleNameEdit.length() > 0) {
            S.bleNameEdit.remove(S.bleNameEdit.length() - 1);
        }

        M5Cardputer.Display.fillRect(0, 42, Dim::SCREEN_W, 30, BLACK);
        printAt(5, 48, ">", GREEN);
        printAt(18, 48, trimToWidth(S.bleNameEdit + "_", 34), WHITE);
        return;
    }

    for (auto c : ks.word) {
        if (c >= 32 && c <= 126 && S.bleNameEdit.length() < 20) {
            S.bleNameEdit += c;
        }
    }

    M5Cardputer.Display.fillRect(0, 42, Dim::SCREEN_W, 30, BLACK);
    printAt(5, 48, ">", GREEN);
    printAt(18, 48, trimToWidth(S.bleNameEdit + "_", 34), WHITE);
    return;
}

// BLE INFO MODE
if (S.uiMode == UI_BLE_INFO) {
    if (keyEsc(ks)) {
        S.uiMode = UI_LAB;
        showLabScreen();
        return;
    }

    if (keyUp(ks) || (ks.fn && keyUp(ks))) {
        if (S.bleMenuSel > 0) S.bleMenuSel--;
        showBleScreen();
        return;
    }

    if (keyDown(ks) || (ks.fn && keyDown(ks))) {
        if (S.bleMenuSel < 1) S.bleMenuSel++;
        showBleScreen();
        return;
    }

    if (ks.enter) {
        if (S.bleMenuSel == 1) {
            if (bleAdvertisingRunning() || bleServerRunning()) {
                soundError();
                showBleScreen();
                return;
            }

            S.uiMode = UI_BLE_NAME_EDIT;
            S.bleNameEdit = bleDeviceName();

            M5Cardputer.Display.fillScreen(BLACK);
            drawStatusBar();

            printAt(5, Dim::HIST_Y0, "SET BLE NAME", CYAN);
            printAt(5, 48, ">", GREEN);
            printAt(18, 48, trimToWidth(S.bleNameEdit + "_", 34), WHITE);
            printAt(5, Dim::PROMPT_Y, "ENTER SAVE  ESC BACK  BSP DEL", DARKGREY);
            return;
        }

        if (bleAdvertisingRunning() || bleServerRunning()) {
            bleAdvertisingStop();
            bleServerStop();
            soundSuccess();
            delay(300);
            showBleScreen();
            return;
        }

        networkDisconnect();
        delay(500);

        if (bleServerStart()) {
            bleAdvertisingStart();
            soundSuccess();
        } else {
            soundError();
        }

        delay(300);
        showBleScreen();
        return;
    }

    return;
}
    // SHELL MODE
    if (keyUp(ks)) {
        S.scrollOffset++;
        renderAll();
        return;
    }

    if (keyDown(ks)) {
        S.scrollOffset--;
        renderAll();
        return;
    }

    if (keyLeft(ks)) {
        S.cmdOffset--;
        drawBottomBar();
        return;
    }

    if (keyRight(ks)) {
        S.cmdOffset++;
        drawBottomBar();
        return;
    }

    if (ks.enter) {
        executeCommand(S.inputLine);
        return;
    }

    if (keyBackspace(ks)) {
        if (S.inputLine.length() > 0) {
            S.inputLine.remove(S.inputLine.length() - 1);
            autoCmdEnd();
            drawBottomBar();
        }
        return;
    }

    for (auto c : ks.word) {
        if (c >= 32 && c <= 126 && S.inputLine.length() < Limit::INPUT_MAX) {
            S.inputLine += c;
            autoCmdEnd();
            drawBottomBar();
        }
    }
}
