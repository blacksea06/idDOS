#pragma once
#include <Arduino.h>

namespace PinRegistry {

enum PinRole : uint8_t {
    ROLE_LOCKED,
    ROLE_USER_SAFE,
    ROLE_SHARED_CAUTION,
    ROLE_EXT_CAUTION
};

struct PinInfo {
    int pin;
    const char* name;
    PinRole role;
    const char* note;
    bool gpio;
    bool pwm;
    bool softI2C;
    bool uartRx;
    bool uartTx;
    bool spi;
};

constexpr int PIN_GROVE_A = 1;   // Grove white
constexpr int PIN_GROVE_B = 2;   // Grove yellow
constexpr int PIN_UART_RX = 13;  // EXT UART_RX
constexpr int PIN_UART_TX = 15;  // EXT UART_TX
constexpr int PIN_EXT_SPI_CS = 5;
constexpr int PIN_EXT_SPI_MOSI = 14;
constexpr int PIN_EXT_SPI_MISO = 39;
constexpr int PIN_EXT_SPI_SCK  = 40;
constexpr int PIN_SD_CS = 12;

const PinInfo* info(int pin);
bool isSystemLocked(int pin);
bool isUserGpio(int pin);
bool canPwm(int pin);
bool canSoftI2C(int sda, int scl);
bool canUart(int rx, int tx);
bool canExtSpi(int sck, int miso, int mosi, int cs);
String pinName(int pin);
String pinNote(int pin);
String pinWarning(int pin);

} // namespace PinRegistry
