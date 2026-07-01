#pragma once
#include <Arduino.h>

namespace BusManager {

enum BusId : uint8_t {
    BUS_SAFE_I2C,
    BUS_SYSTEM_I2C,
    BUS_SPI_EXT,
    BUS_UART_EXT,
    BUS_GPIO_TOOL,
    BUS_PWM_TOOL,
    BUS_AUDIO,
    BUS_RADIO_WIFI,
    BUS_RADIO_BLE,
    BUS_COUNT
};

bool acquire(BusId bus, const String& owner);
void release(BusId bus, const String& owner = "");
bool busy(BusId bus);
String owner(BusId bus);
String busName(BusId bus);
String lastError();
void reset();

} // namespace BusManager
