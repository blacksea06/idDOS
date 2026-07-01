# idDOS v1.0.3 Audio Path Fix

Цель: исправить ситуацию, когда WAV-файл открыт, счётчик playback идёт, но звука нет.

## Изменения

- Перед WAV playback теперь явно выключается `Mic` через `M5Cardputer.Mic.end()`.
- После выключения микрофона заново поднимается `Speaker` через `Speaker.end()` -> `Speaker.begin()`.
- Громкость playback поднята до 255 для проверки на железе.
- После `stopRecord()` capture path освобождается сразу.
- Boot chime возвращён ближе к старому рабочему алгоритму: без агрессивного `Speaker.stop()` в `soundBegin()`, задержка 350 ms, умеренная громкость.
- Добавлена скрытая диагностическая команда:

```text
beep
soundtest
```

Если `beep` молчит — проблема не в WAV player, а в speaker/codec/настройке/3.5mm jack.
Если `beep` звучит, а WAV нет — проблема в `playRaw()`/формате PCM или в backend playback.

## Проверка

1. Прошить v1.0.3.
2. В shell выполнить `beep`.
3. Если beep звучит — открыть WAV и нажать ENTER.
4. Проверить, что в 3.5mm jack ничего не вставлено: на Cardputer-Adv jack отключает встроенный speaker amplifier.
