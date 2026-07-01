# idDOS v1.0.12 deep-opt audit

Цель патча: убрать реальные зоны риска без переписывания всей ОС и без выдуманных «улучшений», которые нельзя подтвердить кодом или железом.

## Опора на железо Cardputer-Adv

Патч сверялся с Cardputer-Adv: ESP32-S3FN8, 8MB flash, ST7789V2 240x135, 56-key keyboard, ES8311 audio, BMI270 IMU, TCA8418 keyboard, microSD на G12/G14/G40/G39, системный I2C на G8/G9, Grove G1/G2, EXT SPI G5/G14/G39/G40, UART G13/G15.

## Что изменено

### 1. PlatformIO baseline

Файл: `platformio.ini`

- Зафиксирован `espressif32@6.7.0`.
- Поднят upload speed до `1500000`.
- Добавлены USB CDC flags:
  - `ARDUINO_USB_CDC_ON_BOOT=1`
  - `ARDUINO_USB_MODE=1`
- `CORE_DEBUG_LEVEL=0` для release-прошивки.
- `M5Cardputer` берётся из официального GitHub URL, чтобы не зависеть от случайной версии registry.

### 2. Ball app больше не блокирует ОС

Файл: `src/shell/shell_ble_ball_update.cpp`

Старая версия запускала `while(true)` внутри `startBallApp()`. Это блокировало shell, батарейный тик, аудио/UI tick и любой возврат. Новая версия:

- `startBallApp()` только инициализирует состояние;
- `updateBallApp()` вызывается из `shellUpdate()`;
- выход по `ESC` или `ENTER`;
- кадры ограничены до ~60 FPS через `millis()` без жёсткого вечного цикла.

### 3. Лимитированное чтение строк

Файлы:

- `src/core/config.cpp`
- `src/shell/shell_fs.cpp`
- `src/shell/shell_editor.cpp`

Старый `readStringUntil('\n')` мог съесть слишком длинную строку и раздуть heap. Теперь чтение ограничено:

- config line: 160 символов;
- cat line: `Limit::CAT_LEN`;
- editor line: `Limit::EDITOR_MAX_COLS`.

Это снижает риск фрагментации и зависаний на повреждённых/огромных файлах.

### 4. Рекурсивные SD операции получили ограничение глубины

Файл: `src/shell/shell_fs.cpp`

- `removeRecursive()` и `copyDirRecursive()` теперь используют depth-limit 12.
- Это защищает стек ESP32-S3 от слишком глубоких директорий.

### 5. WLAN scan стал чище

Файл: `src/lab/network.cpp`

- Убраны лишние startup delays перед scan.
- Добавлен cap результата до 24 сетей.
- Добавлен `WiFi.scanDelete()` после scan, чтобы освободить память Wi-Fi scan buffer.
- Добавлен dedupe по `SSID+BSSID`.
- `networkConnect()` теперь сначала сбрасывает старое состояние и корректно подключается к OPEN сетям без пустого password overload.

### 6. WLAN disconnect теперь относится к выбранной сети

Файлы:

- `src/shell/shell_wlan.cpp`
- `src/shell/shell_ble_ball_update.cpp`

Раньше любая выбранная сеть показывала `DISCONNECT`, если Wi-Fi уже подключён вообще. Теперь `DISCONNECT` показывается только если выбранная сеть совпадает с `networkCurrentSSID()`.

### 7. Unlock получил anti-bruteforce задержку

Файл: `src/main.cpp`

- Ошибка пароля даёт 750 мс паузы.
- После 5 ошибок пауза 5 секунд.
- Успешный вход сбрасывает счётчик.

Это не делает устройство «военным сейфом», но убирает тупой быстрый перебор с клавиатуры.

## Что сознательно не тронуто

- Не переписан весь ShellState на fixed buffers: риск регрессии выше пользы для v1.0.12.
- Не заменён SHA-256 config password на полноценный KDF: это потребует совместимости и миграции формата. Текущий вариант уже хранит salt+hash, не plaintext.
- Не изменены GPIO pin roles: текущий pin registry совпадает с заявленной картой Cardputer-Adv.
- Не включён BLE по умолчанию: Wi-Fi/BLE coexistence на ESP32-S3 лучше держать вручную.

## Следующий уровень для 10/10

1. Ввести App interface: `begin/update/end` для всех lab apps.
2. Разделить `shell_ble_ball_update.cpp` на `shell_update.cpp`, `ble_ui.cpp`, `ball_app.cpp`.
3. Увести shell input/history/editor из `String` в малые bounded buffers.
4. Добавить SD transaction/error overlay.
5. Добавить unit-style host tests для pure string/path функций.
6. Ввести watchdog-friendly async Wi-Fi connect.

## Итог

Патч v1.0.12 не ломает модель проекта, но убирает самые неприятные реальные риски: вечный цикл, неограниченное чтение строк, рекурсивный стек, Wi-Fi scan память, неверный WLAN disconnect UX и быстрый перебор unlock.
