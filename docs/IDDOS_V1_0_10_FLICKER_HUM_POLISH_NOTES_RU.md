# idDOS v1.0.10 — flicker + REC hum polish

База: `iddos_v1_0_9_aesthetic_polish.zip`, которая была собрана поверх рабочего `iddos_v1_0_8_sorted_keyboard_rollback_fix.zip`.

## Что исправлено

### 1. Плеер больше не должен мерцать всем меню

Раньше динамическая отрисовка плеера очищала весь блок `WAV DECK` каждые ~500 мс:

- рамка;
- play/pause;
- таймер;
- progress bar;
- volume bar.

На LCD это выглядело как рябь/мигание меню.

Теперь рамка плеера рисуется один раз, а обновляются только маленькие зоны:

- строка PLAY/PAUSE + время;
- progress bar;
- volume bar.

### 2. Нижний курсор `_` больше не перерисовывает нижнюю строку

Раньше мигание курсора вызывало `drawBottomBar()`, из-за чего вместе с `_` мигала серая пунктирная separator-линия.

Теперь для мигания используется отдельная функция:

```cpp
void drawCmdCursorOnly();
```

Она обновляет только ячейку курсора, не трогая всю нижнюю панель.

### 3. Гул после записи

После остановки записи теперь сразу вызывается аудио-idle:

```cpp
AudioLab::idleSilence();
```

Также убран короткий `soundSuccess()` при старте записи, потому что он мог оставлять speaker/amp включённым во время работы mic/capture path.

`idleSilence()` теперь жёстче гасит аудиотракт:

```cpp
Mic.end();
Speaker.stop();
Speaker.end();
```

Системные звуки теперь сами поднимают `Speaker.begin()` перед tone, чтобы нормально работать после `Speaker.end()`.

## Изменённые файлы

```text
src/shell/shell_core.cpp
src/shell/shell_audio.cpp
src/shell/shell_ble_ball_update.cpp
src/lab/audio_lab.cpp
src/core/sound.cpp
include/shell/shell_internal.h
include/core/version.h
```

## Версия

```text
1.0.10
```
