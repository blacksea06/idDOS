# idDOS v1.0 analysis

## Philosophy

idDOS is a command operating environment for Cardputer-Adv. `id` is the impulse to check, connect, build, record, and bring hardware alive. `DOS` is the command path to the machine. The rule stays: desire becomes command.

## Release scope

v1.0 keeps the existing command OS, SD workspace, editor, secure config, WLAN/BLE labs, REC/AUDIO, WAV player, and HW Tools. The release refactors the shell into segments and keeps all operations inside the Cardputer-Adv hardware limits.

## Navigation

- ENTER: select, confirm, execute, record/stop, play/stop.
- UP/DOWN: menu selection or scroll.
- LEFT/RIGHT: value adjustment or shell command horizontal scroll.
- ESC/FN+ESC: exit/back where a mode supports it.

BALL is intentionally left unchanged per project decision.

## Functional map

### Boot / auth

- boot screen
- SD/layout check
- config load
- unlock screen
- password mask
- password hash/salt check
- lock command
- reboot command

### Shell

- prompt and breadcrumb
- history with limit
- status bar with path, battery, Wi-Fi indicator
- commands: dir, cd, cd.., new, mkdir, del, copy, paste, open, cat, cls, help, info, lock, reboot, ver, about
- hardware commands: i2c, spi, uart, gpio, pwm, dac, logic, oled, display, rtc

### SD workspace

- /scripts
- /apps
- /data
- /data/tmp
- /data/idchat
- /audio
- /backup
- /docs

### File operations

- create files
- create directories
- delete file or directory using del
- recursive directory delete
- copy file/directory
- recursive directory copy
- conflict-safe paste names
- safe name validation

### Editor

- .txt and .id editor
- cursor movement
- scroll
- line numbers
- save/no-save/cancel menu

### Audio

- LAB REC
- live waveform
- WAV PCM mono 16 kHz recording
- save/discard flow
- save to /audio
- open .wav player
- play/stop

### WLAN

- scan networks
- RSSI/channel/security/BSSID
- connect/disconnect
- IP/gateway/DNS info

### BLE

- GATT server
- advertising start/stop
- device name edit
- NUS-like service/characteristics

### HW Tools

Included:

- I2C Scanner on G1/G2 only
- SPI Test on EXT SPI with G5 CS
- UART Console on G13/G15
- GPIO Monitor on G1/G2
- PWM Generator on G1/G2
- PWM-DAC on G2
- Logic Probe on G1/G2
- OLED Test on G1/G2 I2C
- Display Test for internal LCD
- RTC/Time soft info

Not included by design:

- ADC Monitor
- I2C Sniffer

## Hardware safety policy

- G1/G2: safe Grove LAB port.
- G13/G15: UART console.
- G5: EXT SPI CS.
- G14/G39/G40: shared SPI/SD lines, guarded.
- G8/G9: locked system I2C for keyboard, IMU, ES8311.
- G10: locked battery ADC.
- G11: locked keyboard INT.
- G12: locked SD CS.
- G33-G38: locked LCD.
- G41-G43/G46: locked audio I2S.
- G44: locked IR TX.

## Known risk areas

- Audio API depends on the exact M5Cardputer/M5Unified version.
- LEDC API in Arduino-ESP32 2.x and 3.x differs; wrappers are included in hardware_tools.cpp.
- BALL remains unchanged and should be revisited later only if the project decision changes.
- True PlatformIO build must be run on the target project with actual libraries.

## Shell segmentation

- shell.cpp: anchor file only
- shell_internal.h: shared declarations
- shell_state.cpp: runtime state
- shell_core.cpp: render/history/status/core helpers
- shell_menus.cpp: welcome/help/info/root/options/lab
- shell_wlan.cpp: WLAN UI
- shell_fs.cpp: SD/file operations
- shell_audio.cpp: REC/player UI
- shell_editor.cpp: text editor
- shell_commands.cpp: command parser and public shell start/lock API
- shell_ble_ball_update.cpp: BLE screen, BALL, shellUpdate dispatcher
