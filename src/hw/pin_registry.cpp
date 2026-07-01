#include "pin_registry.h"

namespace PinRegistry {

static const PinInfo PIN_TABLE[] = {
    {1,  "G1",  ROLE_USER_SAFE,     "Grove white / LAB_A", true, true, true, false, false, false},
    {2,  "G2",  ROLE_USER_SAFE,     "Grove yellow / LAB_B", true, true, true, false, false, false},
    {3,  "G3",  ROLE_EXT_CAUTION,   "EXT RESET label", true, false, false, false, false, false},
    {4,  "G4",  ROLE_EXT_CAUTION,   "EXT INT label", true, false, false, false, false, false},
    {5,  "G5",  ROLE_SHARED_CAUTION,"EXT SPI CS", true, true, false, false, false, true},
    {6,  "G6",  ROLE_EXT_CAUTION,   "EXT BUSY label", true, true, false, false, false, false},
    {8,  "G8",  ROLE_LOCKED,        "system I2C SDA: keyboard/IMU/audio", false, false, false, false, false, false},
    {9,  "G9",  ROLE_LOCKED,        "system I2C SCL: keyboard/IMU/audio", false, false, false, false, false, false},
    {10, "G10", ROLE_LOCKED,        "battery ADC", false, false, false, false, false, false},
    {11, "G11", ROLE_LOCKED,        "keyboard INT", false, false, false, false, false, false},
    {12, "G12", ROLE_LOCKED,        "microSD CS", false, false, false, false, false, false},
    {13, "G13", ROLE_USER_SAFE,     "EXT UART RX / GPIO", true, false, false, true, false, false},
    {14, "G14", ROLE_SHARED_CAUTION,"microSD MOSI / EXT SPI MOSI", false, false, false, false, false, true},
    {15, "G15", ROLE_USER_SAFE,     "EXT UART TX / GPIO", true, false, false, false, true, false},
    {33, "G33", ROLE_LOCKED,        "LCD reset", false, false, false, false, false, false},
    {34, "G34", ROLE_LOCKED,        "LCD RS/DC", false, false, false, false, false, false},
    {35, "G35", ROLE_LOCKED,        "LCD MOSI", false, false, false, false, false, false},
    {36, "G36", ROLE_LOCKED,        "LCD SCK", false, false, false, false, false, false},
    {37, "G37", ROLE_LOCKED,        "LCD CS", false, false, false, false, false, false},
    {38, "G38", ROLE_LOCKED,        "LCD backlight", false, false, false, false, false, false},
    {39, "G39", ROLE_SHARED_CAUTION,"microSD MISO / EXT SPI MISO", false, false, false, false, false, true},
    {40, "G40", ROLE_SHARED_CAUTION,"microSD SCK / EXT SPI SCK", false, false, false, false, false, true},
    {41, "G41", ROLE_LOCKED,        "audio I2S SCLK", false, false, false, false, false, false},
    {42, "G42", ROLE_LOCKED,        "audio I2S DSDIN", false, false, false, false, false, false},
    {43, "G43", ROLE_LOCKED,        "audio I2S LRCK", false, false, false, false, false, false},
    {44, "G44", ROLE_LOCKED,        "IR TX", false, false, false, false, false, false},
    {46, "G46", ROLE_LOCKED,        "audio I2S ASDOUT", false, false, false, false, false, false},
};

const PinInfo* info(int pin) {
    for (const auto& p : PIN_TABLE) {
        if (p.pin == pin) return &p;
    }
    return nullptr;
}

bool isSystemLocked(int pin) {
    const PinInfo* p = info(pin);
    return !p || p->role == ROLE_LOCKED;
}

bool isUserGpio(int pin) {
    const PinInfo* p = info(pin);
    return p && p->gpio && p->role != ROLE_LOCKED;
}

bool canPwm(int pin) {
    const PinInfo* p = info(pin);
    return p && p->pwm && p->role != ROLE_LOCKED;
}

bool canSoftI2C(int sda, int scl) {
    return sda == PIN_GROVE_A && scl == PIN_GROVE_B;
}

bool canUart(int rx, int tx) {
    return rx == PIN_UART_RX && tx == PIN_UART_TX;
}

bool canExtSpi(int sck, int miso, int mosi, int cs) {
    return sck == PIN_EXT_SPI_SCK && miso == PIN_EXT_SPI_MISO &&
           mosi == PIN_EXT_SPI_MOSI && cs == PIN_EXT_SPI_CS;
}

String pinName(int pin) {
    const PinInfo* p = info(pin);
    return p ? String(p->name) : String("G") + String(pin);
}

String pinNote(int pin) {
    const PinInfo* p = info(pin);
    return p ? String(p->note) : String("unknown pin");
}

String pinWarning(int pin) {
    const PinInfo* p = info(pin);
    if (!p) return "PIN NOT IN REGISTRY";
    if (p->role == ROLE_LOCKED) return "LOCKED: " + String(p->note);
    if (p->role == ROLE_SHARED_CAUTION) return "SHARED: " + String(p->note);
    if (p->role == ROLE_EXT_CAUTION) return "CAUTION: " + String(p->note);
    return "SAFE: " + String(p->note);
}

} // namespace PinRegistry
