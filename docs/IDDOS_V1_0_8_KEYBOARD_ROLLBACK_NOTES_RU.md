# idDOS v1.0.8 — Keyboard rollback / Cardputer physical mapping

Этот патч исправляет ошибку v1.0.7, где управление было переведено на догадку FN+W/S/A/D и из-за этого реальные сочетания клавиш на Cardputer-Adv сломались.

## Что исправлено

Файл:

```text
include/core/keys_compat.h
```

Теперь используются физические подписи Cardputer:

```text
ESC       верхняя левая клавиша ESC / ~
UP        FN + ;
DOWN      FN + .
LEFT      FN + ,
RIGHT     FN + /
BACKSPACE физическая del/backspace
ENTER     ok / enter
```

Дополнительно оставлены fallback-варианты:

```text
HID arrow codes
FN+W/S/A/D
```

Но основной путь теперь снова соответствует реальной клавиатуре Cardputer, а не придуманной PC-раскладке.

## Структура

Проект остаётся отсортированным:

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
