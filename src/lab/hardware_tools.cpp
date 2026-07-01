#include <M5Cardputer.h>
#include "keys_compat.h"
#include <Wire.h>
#include <SPI.h>
#include <time.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#include "hardware_tools.h"
#include "pin_registry.h"
#include "bus_manager.h"
#include "system_log.h"

namespace HardwareTools {

namespace {
constexpr int W = 240;
constexpr int H = 135;
constexpr int X0 = 4;
constexpr int STATUS_H = 15;
constexpr int Y0 = 17;
constexpr int LINE_H = 13;
constexpr int SEP_Y = 112;
constexpr int PROMPT_Y = 121;

constexpr int SDA_SAFE = PinRegistry::PIN_GROVE_A;
constexpr int SCL_SAFE = PinRegistry::PIN_GROVE_B;
constexpr int SPI_SCK  = PinRegistry::PIN_EXT_SPI_SCK;
constexpr int SPI_MISO = PinRegistry::PIN_EXT_SPI_MISO;
constexpr int SPI_MOSI = PinRegistry::PIN_EXT_SPI_MOSI;
constexpr int SPI_CS   = PinRegistry::PIN_EXT_SPI_CS;
constexpr int UART_RX  = PinRegistry::PIN_UART_RX;
constexpr int UART_TX  = PinRegistry::PIN_UART_TX;

TwoWire SafeWire(1);
SPIClass ExtSPI(FSPI);

bool gActive = false;
bool gExit = false;
ToolId currentTool = TOOL_I2C;
int menuSel = 0;
int menuScroll = 0;
int gpioPin = PinRegistry::PIN_GROVE_A;
int gpioMode = 0; // 0 input, 1 pullup
int pwmPin = PinRegistry::PIN_GROVE_A;
int pwmFreq = 1000;
int pwmDuty = 50;
bool pwmRunning = false;
int dacPin = PinRegistry::PIN_GROVE_B;
float dacVolt = 1.65f;
bool dacRunning = false;
bool uartRunning = false;
uint32_t uartBaud = 115200;
String uartTxLine;
String uartRxText;
int logicPin = PinRegistry::PIN_GROVE_A;
int logicLast = LOW;
uint32_t logicEdges = 0;
uint32_t logicWindowStart = 0;
uint32_t logicWindowEdges = 0;
float logicFreq = 0.0f;
int displayPattern = 0;
uint32_t lastDynamicMs = 0;
String i2cResult;
String spiResult;
String oledResult;
bool i2cScanning = false;
uint8_t i2cAddr = 0x08;
uint8_t i2cFoundCount = 0;
String i2cFoundList;
uint32_t i2cLastStepMs = 0;

const char* toolName(ToolId t) {
    switch (t) {
        case TOOL_I2C:     return "I2C";
        case TOOL_SPI:     return "SPI";
        case TOOL_UART:    return "UART";
        case TOOL_GPIO:    return "GPIO";
        case TOOL_PWM:     return "PWM";
        case TOOL_DAC:     return "DAC";
        case TOOL_LOGIC:   return "LOGIC";
        case TOOL_OLED:    return "OLED";
        case TOOL_DISPLAY: return "DISPLAY";
        case TOOL_RTC:     return "RTC";
        default:           return "TOOL";
    }
}

const char* toolTag(ToolId t) {
    switch (t) {
        case TOOL_I2C:     return "<EXP>";
        case TOOL_SPI:     return "<TEST>";
        case TOOL_UART:    return "<TTY>";
        case TOOL_GPIO:    return "<MON>";
        case TOOL_PWM:     return "<GEN>";
        case TOOL_DAC:     return "<PWM>";
        case TOOL_LOGIC:   return "<PROBE>";
        case TOOL_OLED:    return "<TEST>";
        case TOOL_DISPLAY: return "<LCD>";
        case TOOL_RTC:     return "<TIME>";
        default:           return "";
    }
}

void printAt(int x, int y, const String& s, uint16_t color = WHITE) {
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(color, BLACK);
    M5Cardputer.Display.setCursor(x, y);
    M5Cardputer.Display.print(s);
}

String fit(const String& s, int maxChars) {
    if ((int)s.length() <= maxChars) return s;
    if (maxChars <= 3) return s.substring(0, maxChars);
    return s.substring(0, maxChars - 3) + "...";
}

void clear() {
    M5Cardputer.Display.fillScreen(BLACK);
}

void header(const String& title) {
    clear();
    printAt(4, 2, "id:\\>LAB\\TOOLS\\", GREEN);
    printAt(5, Y0, title, CYAN);
}

void footer(const String& text) {
    M5Cardputer.Display.fillRect(0, SEP_Y, W, H - SEP_Y, BLACK);
    printAt(0, SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, PROMPT_Y, fit(text, 38), WHITE);
}

void drawMenu() {
    header("HW TOOLS");
    // 6 rows fit above footer on 240x135 without the last row falling under the prompt.
    int visible = 6;
    if (menuSel < 0) menuSel = 0;
    if (menuSel >= TOOL_COUNT) menuSel = TOOL_COUNT - 1;
    if (menuSel < menuScroll) menuScroll = menuSel;
    if (menuSel >= menuScroll + visible) menuScroll = menuSel - visible + 1;
    if (menuScroll < 0) menuScroll = 0;

    int y = 34;
    for (int i = 0; i < visible; ++i) {
        int idx = menuScroll + i;
        if (idx >= TOOL_COUNT) break;
        bool sel = idx == menuSel;
        printAt(5, y, sel ? ">" : " ", GREEN);
        printAt(18, y, toolName((ToolId)idx), sel ? GREEN : WHITE);
        printAt(96, y, toolTag((ToolId)idx), DARKGREY);
        y += LINE_H;
    }
    footer("ENTER open  ESC lab");
}

void releaseToolBuses() {
    BusManager::release(BusManager::BUS_SAFE_I2C, "I2C");
    BusManager::release(BusManager::BUS_SPI_EXT, "SPI");
    BusManager::release(BusManager::BUS_UART_EXT, "UART");
    BusManager::release(BusManager::BUS_GPIO_TOOL, "GPIO");
    BusManager::release(BusManager::BUS_PWM_TOOL, "PWM");
    BusManager::release(BusManager::BUS_PWM_TOOL, "DAC");
    BusManager::release(BusManager::BUS_GPIO_TOOL, "LOGIC");
}

void stopPwmPin(int pin, int channel) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin, 0);
    ledcDetach(pin);
#else
    ledcWrite(channel, 0);
    ledcDetachPin(pin);
#endif
}

void applyPwm(int pin, int freq, int duty, int channel, bool& running) {
    if (!PinRegistry::canPwm(pin)) return;
    duty = constrain(duty, 0, 100);
    freq = constrain(freq, 1, 40000);
    const int res = 10;
    uint32_t val = ((1 << res) - 1) * duty / 100;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(pin, freq, res);
    ledcWrite(pin, val);
#else
    ledcSetup(channel, freq, res);
    ledcAttachPin(pin, channel);
    ledcWrite(channel, val);
#endif
    running = true;
}

void drawSafetyLine(const String& s) {
    printAt(5, 100, fit(s, 37), YELLOW);
}

void redrawI2C();

void startI2CScan() {
    if (i2cScanning) return;

    // v1.0.5: The I2C scanner is kept as an experimental tool only.
    // On some Cardputer-Adv / library combinations Wire.endTransmission()
    // can block hard when the external Grove bus is floating, shorted, or
    // has a problematic module attached. A release build must not look hung.
    // Therefore we do a preflight and refuse to scan a bus that is not idle.
    pinMode(SDA_SAFE, INPUT_PULLUP);
    pinMode(SCL_SAFE, INPUT_PULLUP);
    delay(5);

    if (digitalRead(SDA_SAFE) == LOW || digitalRead(SCL_SAFE) == LOW) {
        i2cResult = "BUS LOW / CHECK WIRES";
        redrawI2C();
        return;
    }

    if (!BusManager::acquire(BusManager::BUS_SAFE_I2C, "I2C")) {
        i2cResult = BusManager::lastError();
        redrawI2C();
        return;
    }

    SafeWire.begin(SDA_SAFE, SCL_SAFE, 50000);
    SafeWire.setTimeOut(3);
    i2cScanning = true;
    i2cAddr = 0x08;
    i2cFoundCount = 0;
    i2cFoundList = "";
    i2cLastStepMs = 0;
    i2cResult = "SCANNING 0x08";
    redrawI2C();
}

void cancelI2CScan() {
    if (!i2cScanning) return;
    i2cScanning = false;
    BusManager::release(BusManager::BUS_SAFE_I2C, "I2C");
    i2cResult = "SCAN CANCELLED";
}

void updateI2CScan() {
    if (!i2cScanning) return;
    if (millis() - i2cLastStepMs < 60) return;
    i2cLastStepMs = millis();

    if (i2cAddr > 0x77) {
        i2cScanning = false;
        BusManager::release(BusManager::BUS_SAFE_I2C, "I2C");
        if (i2cFoundCount == 0) i2cResult = "NO DEVICES";
        else i2cResult = String(i2cFoundCount) + " FOUND: " + i2cFoundList;
        redrawI2C();
        return;
    }

    SafeWire.beginTransmission(i2cAddr);
    uint8_t e = SafeWire.endTransmission();
    if (e == 0) {
        if (i2cFoundCount) i2cFoundList += " ";
        if (i2cAddr < 16) i2cFoundList += "0";
        i2cFoundList += String(i2cAddr, HEX);
        i2cFoundList.toUpperCase();
        i2cFoundCount++;
    }

    char buf[24];
    snprintf(buf, sizeof(buf), "SCANNING 0x%02X", i2cAddr);
    i2cResult = String(buf);
    i2cAddr++;
    redrawI2C();
}

void openI2C() {
    currentTool = TOOL_I2C;
    i2cScanning = false;
    i2cResult = "ENTER SCAN";
    header("I2C SCANNER");
    printAt(5, 38, "EXP SAFE GROVE", YELLOW);
    printAt(5, 52, "SDA G1  SCL G2", WHITE);
    printAt(5, 66, "ESC cancels scan", DARKGREY);
    printAt(5, 84, fit(i2cResult, 37), YELLOW);
    footer("ENTER scan EXP  ESC tools");
}

void redrawI2C() {
    M5Cardputer.Display.fillRect(0, 82, W, 22, BLACK);
    printAt(5, 84, fit(i2cResult, 37), YELLOW);
}

void openSPI() {
    currentTool = TOOL_SPI;
    spiResult = "ENTER JEDEC ID";
    header("SPI TEST");
    printAt(5, 36, "EXT SPI SHARES SD BUS", YELLOW);
    printAt(5, 52, "SCK G40  MOSI G14", WHITE);
    printAt(5, 66, "MISO G39 CS G5", WHITE);
    printAt(5, 88, fit(spiResult, 37), CYAN);
    footer("ENTER test  ESC tools");
}

void redrawSPI() {
    M5Cardputer.Display.fillRect(0, 86, W, 20, BLACK);
    printAt(5, 88, fit(spiResult, 37), CYAN);
}

void runSpiJedec() {
    if (!BusManager::acquire(BusManager::BUS_SPI_EXT, "SPI")) {
        spiResult = BusManager::lastError();
        redrawSPI();
        return;
    }
    pinMode(SPI_CS, OUTPUT);
    digitalWrite(SPI_CS, HIGH);
    ExtSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    ExtSPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(SPI_CS, LOW);
    uint8_t b0 = ExtSPI.transfer(0x9F);
    uint8_t b1 = ExtSPI.transfer(0x00);
    uint8_t b2 = ExtSPI.transfer(0x00);
    uint8_t b3 = ExtSPI.transfer(0x00);
    digitalWrite(SPI_CS, HIGH);
    ExtSPI.endTransaction();
    BusManager::release(BusManager::BUS_SPI_EXT, "SPI");
    char buf[40];
    snprintf(buf, sizeof(buf), "RX %02X %02X %02X %02X", b0, b1, b2, b3);
    spiResult = String(buf);
    redrawSPI();
}

void openUART() {
    currentTool = TOOL_UART;
    uartTxLine = "";
    uartRxText = "";
    if (!BusManager::acquire(BusManager::BUS_UART_EXT, "UART")) {
        uartRxText = BusManager::lastError();
    } else {
        Serial1.begin(uartBaud, SERIAL_8N1, UART_RX, UART_TX);
        uartRunning = true;
        systemLog("UART START " + String(uartBaud));
    }
    header("UART CONSOLE");
    printAt(5, 34, "G13 RX  G15 TX  3V3", YELLOW);
    printAt(5, 48, String(uartBaud) + " 8N1", WHITE);
    footer("ENTER send  BSP del  ESC tools");
}

void closeUART() {
    if (uartRunning) {
        Serial1.end();
        uartRunning = false;
        BusManager::release(BusManager::BUS_UART_EXT, "UART");
        systemLog("UART STOP");
    }
}

void drawUART() {
    M5Cardputer.Display.fillRect(0, 62, W, 48, BLACK);
    printAt(5, 62, "RX:", CYAN);
    printAt(28, 62, fit(uartRxText, 32), WHITE);
    printAt(5, 89, "> " + fit(uartTxLine + "_", 34), GREEN);
}

void updateUART() {
    if (!uartRunning) return;
    while (Serial1.available()) {
        char c = (char)Serial1.read();
        if (c >= 32 && c <= 126) uartRxText += c;
        else if (c == '\n') uartRxText += '|';
        if (uartRxText.length() > 80) uartRxText = uartRxText.substring(uartRxText.length() - 80);
    }
    if (millis() - lastDynamicMs > 200) {
        lastDynamicMs = millis();
        drawUART();
    }
}

void openGPIO() {
    currentTool = TOOL_GPIO;
    gpioPin = PinRegistry::PIN_GROVE_A;
    gpioMode = 0;
    if (!BusManager::acquire(BusManager::BUS_GPIO_TOOL, "GPIO")) {}
    pinMode(gpioPin, INPUT);
    header("GPIO MONITOR");
    footer("UP pin  ENTER pull  ESC tools");
}

void drawGPIO() {
    M5Cardputer.Display.fillRect(0, 34, W, 74, BLACK);
    printAt(5, 36, "PIN  " + PinRegistry::pinName(gpioPin), CYAN);
    printAt(5, 50, gpioMode ? "MODE INPUT_PULLUP" : "MODE INPUT", WHITE);
    printAt(5, 66, String("STATE ") + (digitalRead(gpioPin) ? "HIGH" : "LOW"), digitalRead(gpioPin) ? GREEN : RED);
    drawSafetyLine("MAX GPIO INPUT 3.3V");
}

void openPWM() {
    currentTool = TOOL_PWM;
    pwmPin = PinRegistry::PIN_GROVE_A;
    pwmFreq = 1000;
    pwmDuty = 50;
    pwmRunning = false;
    if (!BusManager::acquire(BusManager::BUS_PWM_TOOL, "PWM")) {}
    header("PWM GENERATOR");
    footer("UP pin  L/R duty  ENTER run");
}

void drawPWM() {
    M5Cardputer.Display.fillRect(0, 34, W, 75, BLACK);
    printAt(5, 36, "PIN  " + PinRegistry::pinName(pwmPin), CYAN);
    printAt(5, 52, "FREQ " + String(pwmFreq) + " Hz", WHITE);
    printAt(5, 66, "DUTY " + String(pwmDuty) + "%", WHITE);
    printAt(5, 82, pwmRunning ? "RUNNING" : "STOPPED", pwmRunning ? GREEN : DARKGREY);
    drawSafetyLine("G1/G2 3.3V PWM OUT");
}

void openDAC() {
    currentTool = TOOL_DAC;
    dacPin = PinRegistry::PIN_GROVE_B;
    dacVolt = 1.65f;
    dacRunning = false;
    if (!BusManager::acquire(BusManager::BUS_PWM_TOOL, "DAC")) {}
    header("DAC / PWM-DAC");
    footer("L/R volt  ENTER run  ESC tools");
}

void drawDAC() {
    M5Cardputer.Display.fillRect(0, 34, W, 75, BLACK);
    printAt(5, 36, "PIN  " + PinRegistry::pinName(dacPin), CYAN);
    printAt(5, 52, "OUT  " + String(dacVolt, 2) + " V", WHITE);
    printAt(5, 66, "TYPE PWM + RC FILTER", YELLOW);
    printAt(5, 82, dacRunning ? "RUNNING" : "STOPPED", dacRunning ? GREEN : DARKGREY);
    drawSafetyLine("NO NATIVE DAC ON S3");
}

void openLOGIC() {
    currentTool = TOOL_LOGIC;
    logicPin = PinRegistry::PIN_GROVE_A;
    logicEdges = 0;
    logicWindowEdges = 0;
    logicWindowStart = millis();
    logicFreq = 0;
    pinMode(logicPin, INPUT);
    logicLast = digitalRead(logicPin);
    if (!BusManager::acquire(BusManager::BUS_GPIO_TOOL, "LOGIC")) {}
    header("LOGIC PROBE");
    footer("UP pin  ENTER reset  ESC tools");
}

void drawLOGIC() {
    M5Cardputer.Display.fillRect(0, 34, W, 75, BLACK);
    int v = digitalRead(logicPin);
    printAt(5, 36, "PIN   " + PinRegistry::pinName(logicPin), CYAN);
    printAt(5, 52, String("STATE ") + (v ? "HIGH" : "LOW"), v ? GREEN : RED);
    printAt(5, 66, "EDGES " + String(logicEdges), WHITE);
    printAt(5, 80, "FREQ  " + String(logicFreq, 1) + " Hz", WHITE);
    drawSafetyLine("3.3V LOGIC ONLY");
}

void updateLogic() {
    int v = digitalRead(logicPin);
    if (v != logicLast) {
        logicLast = v;
        logicEdges++;
        logicWindowEdges++;
    }
    uint32_t now = millis();
    if (now - logicWindowStart >= 1000) {
        logicFreq = logicWindowEdges / 2.0f;
        logicWindowEdges = 0;
        logicWindowStart = now;
        drawLOGIC();
    }
}

void oledCommand(uint8_t addr, uint8_t cmd) {
    SafeWire.beginTransmission(addr);
    SafeWire.write(0x00);
    SafeWire.write(cmd);
    SafeWire.endTransmission();
}

void oledData(uint8_t addr, uint8_t data) {
    SafeWire.beginTransmission(addr);
    SafeWire.write(0x40);
    SafeWire.write(data);
    SafeWire.endTransmission();
}

bool oledInit(uint8_t addr) {
    SafeWire.begin(SDA_SAFE, SCL_SAFE, 100000);
    SafeWire.beginTransmission(addr);
    if (SafeWire.endTransmission() != 0) return false;
    const uint8_t init[] = {0xAE,0x20,0x00,0xB0,0xC8,0x00,0x10,0x40,0x81,0x7F,0xA1,0xA6,0xA8,0x3F,0xA4,0xD3,0x00,0xD5,0x80,0xD9,0xF1,0xDA,0x12,0xDB,0x40,0x8D,0x14,0xAF};
    for (uint8_t c : init) oledCommand(addr, c);
    return true;
}

bool oledPattern(uint8_t addr) {
    if (!oledInit(addr)) return false;
    for (uint8_t page = 0; page < 8; ++page) {
        oledCommand(addr, 0xB0 + page);
        oledCommand(addr, 0x00);
        oledCommand(addr, 0x10);
        for (int x = 0; x < 128; ++x) oledData(addr, (x / 8 + page) & 1 ? 0xAA : 0x55);
    }
    return true;
}

void openOLED() {
    currentTool = TOOL_OLED;
    oledResult = "ENTER TEST 0x3C";
    header("OLED TEST");
    printAt(5, 36, "BUS SAFE G1/G2", WHITE);
    printAt(5, 50, "SSD1306 128x64 I2C", WHITE);
    printAt(5, 74, oledResult, YELLOW);
    footer("ENTER test  ESC tools");
}

void redrawOLED() {
    M5Cardputer.Display.fillRect(0, 70, W, 30, BLACK);
    printAt(5, 74, fit(oledResult, 37), YELLOW);
}

void runOLED() {
    if (!BusManager::acquire(BusManager::BUS_SAFE_I2C, "I2C")) {
        oledResult = BusManager::lastError();
        redrawOLED();
        return;
    }
    bool ok = oledPattern(0x3C);
    if (!ok) ok = oledPattern(0x3D);
    BusManager::release(BusManager::BUS_SAFE_I2C, "I2C");
    oledResult = ok ? "OLED PATTERN SENT" : "OLED NOT FOUND";
    redrawOLED();
}

void drawDisplayPattern() {
    clear();
    if (displayPattern == 0) {
        M5Cardputer.Display.fillRect(0, 0, 40, H, RED);
        M5Cardputer.Display.fillRect(40, 0, 40, H, GREEN);
        M5Cardputer.Display.fillRect(80, 0, 40, H, BLUE);
        M5Cardputer.Display.fillRect(120, 0, 40, H, WHITE);
        M5Cardputer.Display.fillRect(160, 0, 40, H, YELLOW);
        M5Cardputer.Display.fillRect(200, 0, 40, H, CYAN);
    } else if (displayPattern == 1) {
        for (int x = 0; x < W; x += 10) M5Cardputer.Display.drawLine(x, 0, x, H - 1, DARKGREY);
        for (int y = 0; y < H; y += 10) M5Cardputer.Display.drawLine(0, y, W - 1, y, DARKGREY);
        printAt(5, 5, "GRID 240x135", GREEN);
    } else {
        printAt(5, 10, "DISPLAY TEST", CYAN);
        printAt(5, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", WHITE);
        printAt(5, 45, "0123456789", WHITE);
        printAt(5, 65, "ENTER NEXT", GREEN);
        printAt(5, 80, "ESC TOOLS", GREEN);
    }
}

void openDISPLAY() {
    currentTool = TOOL_DISPLAY;
    displayPattern = 0;
    drawDisplayPattern();
}

void openRTC() {
    currentTool = TOOL_RTC;
    header("RTC / TIME");
    footer("ENTER refresh  ESC tools");
}

void drawRTC() {
    M5Cardputer.Display.fillRect(0, 34, W, 75, BLACK);
    uint32_t up = millis() / 1000;
    printAt(5, 36, "SOFT RTC / UPTIME", CYAN);
    printAt(5, 52, "UP " + String(up / 3600) + "h " + String((up / 60) % 60) + "m " + String(up % 60) + "s", WHITE);
    printAt(5, 66, String("BUILD ") + __DATE__, WHITE);
    printAt(5, 80, String("TIME  ") + __TIME__, WHITE);
    drawSafetyLine("NTP LATER VIA WLAN");
}

void openSelectedTool(ToolId t) {
    releaseToolBuses();
    currentTool = t;
    lastDynamicMs = 0;
    switch (t) {
        case TOOL_I2C:     openI2C(); break;
        case TOOL_SPI:     openSPI(); break;
        case TOOL_UART:    openUART(); drawUART(); break;
        case TOOL_GPIO:    openGPIO(); drawGPIO(); break;
        case TOOL_PWM:     openPWM(); drawPWM(); break;
        case TOOL_DAC:     openDAC(); drawDAC(); break;
        case TOOL_LOGIC:   openLOGIC(); drawLOGIC(); break;
        case TOOL_OLED:    openOLED(); break;
        case TOOL_DISPLAY: openDISPLAY(); break;
        case TOOL_RTC:     openRTC(); drawRTC(); break;
        default:           drawMenu(); break;
    }
}

void backToMenu() {
    closeUART();
    if (pwmRunning) { stopPwmPin(pwmPin, 0); pwmRunning = false; }
    if (dacRunning) { stopPwmPin(dacPin, 1); dacRunning = false; }
    releaseToolBuses();
    currentTool = TOOL_COUNT;
    drawMenu();
}

} // namespace

void begin() {
    // No hardware touched here. Tools acquire buses only when opened.
}

void openMenu() {
    gActive = true;
    gExit = false;
    currentTool = TOOL_COUNT;
    drawMenu();
    systemLog("HW TOOLS OPEN");
}

void openTool(ToolId tool) {
    gActive = true;
    gExit = false;
    openSelectedTool(tool);
    systemLog(String("HW TOOL ") + toolName(tool));
}

bool active() { return gActive; }
bool exitRequested() { return gExit; }

void update() {
    if (!gActive) return;

    if (currentTool == TOOL_I2C) updateI2CScan();
    if (currentTool == TOOL_UART) updateUART();
    if (currentTool == TOOL_GPIO && millis() - lastDynamicMs > 200) { lastDynamicMs = millis(); drawGPIO(); }
    if (currentTool == TOOL_LOGIC) updateLogic();
    if (currentTool == TOOL_RTC && millis() - lastDynamicMs > 1000) { lastDynamicMs = millis(); drawRTC(); }

    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return;
    auto ks = M5Cardputer.Keyboard.keysState();

    bool esc = keyEsc(ks) || (ks.fn && keyEsc(ks));

    if (currentTool == TOOL_COUNT) {
        if (esc) {
            gExit = true;
            gActive = false;
            systemLog("HW TOOLS CLOSE");
            return;
        }
        if (keyUp(ks) || (ks.fn && keyUp(ks))) { if (menuSel > 0) menuSel--; drawMenu(); return; }
        if (keyDown(ks) || (ks.fn && keyDown(ks))) { if (menuSel < TOOL_COUNT - 1) menuSel++; drawMenu(); return; }
        if (ks.enter) { openSelectedTool((ToolId)menuSel); return; }
        return;
    }

    if (esc) {
        if (currentTool == TOOL_I2C) cancelI2CScan();
        backToMenu();
        return;
    }

    switch (currentTool) {
        case TOOL_I2C:
            if (ks.enter) { startI2CScan(); redrawI2C(); }
            break;

        case TOOL_SPI:
            if (ks.enter) runSpiJedec();
            break;

        case TOOL_UART:
            if (ks.enter) {
                if (uartRunning && uartTxLine.length()) {
                    Serial1.print(uartTxLine);
                    Serial1.print("\r\n");
                    uartRxText += "|TX:" + uartTxLine;
                    if (uartRxText.length() > 80) uartRxText = uartRxText.substring(uartRxText.length() - 80);
                    uartTxLine = "";
                    drawUART();
                }
            } else if (keyBackspace(ks)) {
                if (uartTxLine.length()) uartTxLine.remove(uartTxLine.length() - 1);
                drawUART();
            } else {
                for (auto c : ks.word) {
                    if (c >= 32 && c <= 126 && uartTxLine.length() < 32) uartTxLine += c;
                }
                drawUART();
            }
            break;

        case TOOL_GPIO:
            if (keyUp(ks) || keyDown(ks) || (ks.fn && keyUp(ks)) || (ks.fn && keyDown(ks))) {
                gpioPin = (gpioPin == PinRegistry::PIN_GROVE_A) ? PinRegistry::PIN_GROVE_B : PinRegistry::PIN_GROVE_A;
                pinMode(gpioPin, gpioMode ? INPUT_PULLUP : INPUT);
                drawGPIO();
            } else if (ks.enter) {
                gpioMode = 1 - gpioMode;
                pinMode(gpioPin, gpioMode ? INPUT_PULLUP : INPUT);
                drawGPIO();
            }
            break;

        case TOOL_PWM:
            if (keyUp(ks) || keyDown(ks) || (ks.fn && keyUp(ks)) || (ks.fn && keyDown(ks))) {
                if (pwmRunning) { stopPwmPin(pwmPin, 0); pwmRunning = false; }
                pwmPin = (pwmPin == PinRegistry::PIN_GROVE_A) ? PinRegistry::PIN_GROVE_B : PinRegistry::PIN_GROVE_A;
                drawPWM();
            } else if (keyLeft(ks) || (ks.fn && keyLeft(ks))) {
                pwmDuty = max(0, pwmDuty - 5);
                if (pwmRunning) applyPwm(pwmPin, pwmFreq, pwmDuty, 0, pwmRunning);
                drawPWM();
            } else if (keyRight(ks) || (ks.fn && keyRight(ks))) {
                pwmDuty = min(100, pwmDuty + 5);
                if (pwmRunning) applyPwm(pwmPin, pwmFreq, pwmDuty, 0, pwmRunning);
                drawPWM();
            } else if (ks.enter) {
                if (pwmRunning) { stopPwmPin(pwmPin, 0); pwmRunning = false; }
                else applyPwm(pwmPin, pwmFreq, pwmDuty, 0, pwmRunning);
                drawPWM();
            }
            break;

        case TOOL_DAC:
            if (keyLeft(ks) || (ks.fn && keyLeft(ks))) {
                dacVolt -= 0.05f; if (dacVolt < 0) dacVolt = 0;
                int duty = (int)((dacVolt / 3.3f) * 100.0f);
                if (dacRunning) applyPwm(dacPin, 62500, duty, 1, dacRunning);
                drawDAC();
            } else if (keyRight(ks) || (ks.fn && keyRight(ks))) {
                dacVolt += 0.05f; if (dacVolt > 3.3f) dacVolt = 3.3f;
                int duty = (int)((dacVolt / 3.3f) * 100.0f);
                if (dacRunning) applyPwm(dacPin, 62500, duty, 1, dacRunning);
                drawDAC();
            } else if (ks.enter) {
                if (dacRunning) { stopPwmPin(dacPin, 1); dacRunning = false; }
                else {
                    int duty = (int)((dacVolt / 3.3f) * 100.0f);
                    applyPwm(dacPin, 62500, duty, 1, dacRunning);
                }
                drawDAC();
            }
            break;

        case TOOL_LOGIC:
            if (keyUp(ks) || keyDown(ks) || (ks.fn && keyUp(ks)) || (ks.fn && keyDown(ks))) {
                logicPin = (logicPin == PinRegistry::PIN_GROVE_A) ? PinRegistry::PIN_GROVE_B : PinRegistry::PIN_GROVE_A;
                pinMode(logicPin, INPUT);
                logicLast = digitalRead(logicPin);
                logicEdges = logicWindowEdges = 0;
                logicWindowStart = millis();
                logicFreq = 0;
                drawLOGIC();
            } else if (ks.enter) {
                logicEdges = logicWindowEdges = 0;
                logicWindowStart = millis();
                logicFreq = 0;
                drawLOGIC();
            }
            break;

        case TOOL_OLED:
            if (ks.enter) runOLED();
            break;

        case TOOL_DISPLAY:
            if (ks.enter) {
                displayPattern = (displayPattern + 1) % 3;
                drawDisplayPattern();
            }
            break;

        case TOOL_RTC:
            if (ks.enter) drawRTC();
            break;

        default:
            break;
    }
}

} // namespace HardwareTools
