# idDOS v1.0.6 — отсортированная структура проекта

Этот архив содержит тот же функционал idDOS v1.0.6, но файлы разложены по нормальной структуре PlatformIO.

## Структура

```text
src/
  main.cpp
  core/
    boot.cpp
    config.cpp
    storage.cpp
    sound.cpp
    system_log.cpp
  shell/
    shell.cpp
    shell_core.cpp
    shell_state.cpp
    shell_commands.cpp
    shell_menus.cpp
    shell_fs.cpp
    shell_editor.cpp
    shell_audio.cpp
    shell_wlan.cpp
    shell_ble_ball_update.cpp
  lab/
    audio_lab.cpp
    bluetooth_lab.cpp
    network.cpp
    hardware_tools.cpp
  hw/
    pin_registry.cpp
    bus_manager.cpp

include/
  core/
    boot.h
    config.h
    storage.h
    sound.h
    system_log.h
    version.h
  shell/
    shell.h
    shell_internal.h
    shell_editor.h
  lab/
    audio_lab.h
    bluetooth_lab.h
    network.h
    hardware_tools.h
  hw/
    pin_registry.h
    bus_manager.h

docs/
  заметки релизов и документация
```

## Важно

В коде оставлены старые короткие include-имена вида:

```cpp
#include "sound.h"
#include "shell_internal.h"
#include "hardware_tools.h"
```

Чтобы это работало после сортировки, в `platformio.ini` добавлены include paths:

```ini
build_flags =
    -Iinclude/core
    -Iinclude/shell
    -Iinclude/lab
    -Iinclude/hw
```

## Правило для следующих версий

- `.cpp` кладём в `src/<module>/`
- `.h` кладём в `include/<module>/`
- документацию и заметки релиза кладём в `docs/`
- новый модуль добавляется парой `src/<module>/<name>.cpp` + `include/<module>/<name>.h`

Такой порядок теперь считаем стандартом idDOS.
