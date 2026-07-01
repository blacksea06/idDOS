# idDOS v1.0 release

Command OS layer for M5Stack Cardputer-Adv. Philosophy: id = desire, DOS = command path; desire becomes command.

## Shell segmentation

- shell.cpp: anchor only
- shell_internal.h: shared state, types, prototypes
- shell_state.cpp: global shell/editor/audio state
- shell_core.cpp: strings, prompt, status bar, history renderer
- shell_menus.cpp: help, info, root/options/lab screens
- shell_wlan.cpp: WLAN list/info/password/connect UI
- shell_fs.cpp: SD file manager, new/mkdir/del/copy/paste/cat/init
- shell_audio.cpp: REC recorder UI and WAV player UI
- shell_editor.cpp: text editor .txt/.id
- shell_commands.cpp: command parser, ver/about/lock/reboot, shellStart
- shell_ble_ball_update.cpp: BLE screen, BALL app unchanged, shellUpdate dispatcher

## Current command map

dir, cd <dir>, cd.., new <file.ext>, mkdir <dir>, del <name>, copy <name>, paste, open <file.ext>, cat <file.ext>, cls, help, info, lock, reboot, ver, about.

Hardware tools: i2c, spi, uart, gpio, pwm, dac, logic, oled, display, rtc. ADC Monitor and I2C Sniffer are intentionally not included.

## Navigation policy

- ENTER: select / confirm / send / play-stop / record-stop
- UP/DOWN: menu navigation / scroll
- LEFT/RIGHT: value adjustment or command line scroll
- ESC or FN+ESC: back/exit where available

## Hardware policy

G1/G2 are the safe Grove LAB port. G13/G15 are UART. EXT SPI uses G40/G14/G39 with G5 CS and is shared with SD, so it is guarded. G8/G9 system I2C is locked because keyboard, IMU, and ES8311 share it. ADC Monitor and I2C Sniffer are not part of v1.0.
