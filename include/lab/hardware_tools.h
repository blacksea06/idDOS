#pragma once
#include <Arduino.h>

namespace HardwareTools {

enum ToolId : uint8_t {
    TOOL_I2C = 0,
    TOOL_SPI,
    TOOL_UART,
    TOOL_GPIO,
    TOOL_PWM,
    TOOL_DAC,
    TOOL_LOGIC,
    TOOL_OLED,
    TOOL_DISPLAY,
    TOOL_RTC,
    TOOL_COUNT
};

void begin();
void openMenu();
void openTool(ToolId tool);
void update();
bool exitRequested();
bool active();

} // namespace HardwareTools
