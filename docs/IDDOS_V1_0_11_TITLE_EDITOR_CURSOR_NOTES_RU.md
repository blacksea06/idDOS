# idDOS v1.0 — title/version + editor live cursor polish

База: `iddos_v1_0_10_flicker_hum_polish.zip`.

## Изменения

- Версия на boot/info возвращена к публичной метке `V1.0`.
- Boot title `idDOS` снова держится слева, версия вынесена вправо и больше не должна визуально съезжать/обрезаться.
- В текстовом редакторе добавлен живой курсор `_`.
- Курсор редактора обновляется точечно: перерисовывается только одна символьная ячейка, без мерцания всего блокнота.

## Затронутые файлы

```text
include/core/version.h
src/core/boot.cpp
include/shell/shell_internal.h
src/shell/shell_editor.cpp
src/shell/shell_ble_ball_update.cpp
```

Публичная версия оставлена как `1.0`, без наращивания номера на экране.
