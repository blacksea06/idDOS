# idDOS v1.0.7 — KeysState compatibility compile-fix

## Причина ошибки

M5Cardputer 1.1.1 `Keyboard_Class::KeysState` не содержит полей:

- `esc`
- `up`
- `down`
- `left`
- `right`
- `backspace`

В этой версии доступны поля `fn`, `enter`, `del`, `space`, `word`, `hid_keys` и модификаторы. Поэтому прямые обращения вида `ks.esc`, `ks.up`, `ks.backspace` ломали сборку.

## Исправление

Добавлен compatibility layer:

```text
include/core/keys_compat.h
```

Он вводит функции:

```cpp
keyEsc(ks)
keyBackspace(ks)
keyUp(ks)
keyDown(ks)
keyLeft(ks)
keyRight(ks)
```

## Навигация Cardputer

Для idDOS приняты безопасные сочетания:

```text
FN + W  = вверх
FN + S  = вниз
FN + A  = влево
FN + D  = вправо
FN + `  = ESC / назад
FN + Backspace = ESC / назад
Backspace = удаление символа
```

Также helper принимает HID-коды стрелок, если будущая версия библиотеки начнет их отдавать.

## Изменённые файлы

```text
include/core/keys_compat.h
src/main.cpp
src/lab/hardware_tools.cpp
src/shell/shell_ble_ball_update.cpp
```

## Структура проекта

Сортированная структура сохранена:

```text
src/core
src/shell
src/lab
src/hw
include/core
include/shell
include/lab
include/hw
```
