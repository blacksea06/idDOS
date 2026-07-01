# idDOS v1.0.6 Sorted Project Compile Fix

## Что исправлено

В отсортированной версии проекта `platformio.ini` был слишком минимальным и не подтягивал зависимости проекта.
Из-за этого компиляция могла падать на первом файле, который включает библиотеку Cardputer:

```text
src/core/sound.cpp
#include <M5Cardputer.h>
```

Типовой симптом:

```text
compilation terminated.
*** [.pio/build/esp32s3/src/core/sound.cpp.o] Error 1
```

## Исправление

В `platformio.ini` добавлены зависимости:

```ini
lib_deps =
    m5stack/M5Cardputer
    h2zero/NimBLE-Arduino
```

Также сохранены include-paths для новой структуры:

```ini
build_flags =
    -Iinclude/core
    -Iinclude/shell
    -Iinclude/lab
    -Iinclude/hw
```

## Структура сохранена

```text
src/main.cpp
src/core/*.cpp
src/shell/*.cpp
src/lab/*.cpp
src/hw/*.cpp

include/core/*.h
include/shell/*.h
include/lab/*.h
include/hw/*.h
```

## Проверка

После замены архива выполнить:

```bash
pio run
```

Если ошибка повторится, нужно смотреть не только хвост, а строки выше `compilation terminated`, особенно `fatal error:`.
