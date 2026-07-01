#include "bus_manager.h"

namespace BusManager {

static String gOwner[BUS_COUNT];
static String gLastError;

String busName(BusId bus) {
    switch (bus) {
        case BUS_SAFE_I2C:   return "SAFE I2C";
        case BUS_SYSTEM_I2C: return "SYSTEM I2C";
        case BUS_SPI_EXT:    return "EXT SPI";
        case BUS_UART_EXT:   return "EXT UART";
        case BUS_GPIO_TOOL:  return "GPIO";
        case BUS_PWM_TOOL:   return "PWM";
        case BUS_AUDIO:      return "AUDIO";
        case BUS_RADIO_WIFI: return "WIFI";
        case BUS_RADIO_BLE:  return "BLE";
        default:             return "BUS";
    }
}

static bool blocks(BusId wanted, BusId held) {
    if (wanted == held) return true;

    // Wi-Fi/BLE/idChat class radio conflicts.
    if ((wanted == BUS_RADIO_WIFI && held == BUS_RADIO_BLE) ||
        (wanted == BUS_RADIO_BLE && held == BUS_RADIO_WIFI)) return true;

    // EXT SPI shares physical lines with microSD. Keep it exclusive from audio
    // recording and any future long SD write tools.
    if ((wanted == BUS_SPI_EXT && held == BUS_AUDIO) ||
        (wanted == BUS_AUDIO && held == BUS_SPI_EXT)) return true;

    // System I2C is keyboard/IMU/audio-control. Do not share with tools.
    if ((wanted == BUS_SYSTEM_I2C && held != BUS_SYSTEM_I2C) ||
        (held == BUS_SYSTEM_I2C && wanted != BUS_SYSTEM_I2C)) return true;

    return false;
}

bool acquire(BusId bus, const String& owner) {
    gLastError = "";
    for (int i = 0; i < BUS_COUNT; ++i) {
        if (gOwner[i].length() == 0) continue;
        if (blocks(bus, (BusId)i)) {
            gLastError = busName(bus) + " BUSY BY " + gOwner[i];
            return false;
        }
    }
    gOwner[bus] = owner;
    return true;
}

void release(BusId bus, const String& owner) {
    if ((int)bus < 0 || bus >= BUS_COUNT) return;
    if (owner.length() == 0 || gOwner[bus] == owner) gOwner[bus] = "";
}

bool busy(BusId bus) {
    if ((int)bus < 0 || bus >= BUS_COUNT) return true;
    return gOwner[bus].length() > 0;
}

String owner(BusId bus) {
    if ((int)bus < 0 || bus >= BUS_COUNT) return "";
    return gOwner[bus];
}

String lastError() { return gLastError; }

void reset() {
    for (int i = 0; i < BUS_COUNT; ++i) gOwner[i] = "";
    gLastError = "";
}

} // namespace BusManager
