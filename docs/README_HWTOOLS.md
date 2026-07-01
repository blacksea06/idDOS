# idDOS v1.0 v1-release

Готовый пакет исходников для M5Stack Cardputer-Adv / PlatformIO.

## Сохранено

- boot / unlock / shell
- SD file manager
- text editor
- commands: dir, cd, cd.., new, mkdir, del, copy, paste, open, cat, cls, help, info, lock, reboot, ver, about
- secure config with salt + SHA-256 hash and legacy PASSWORD migration
- system.log
- WLAN Lab
- BLE GATT Lab
- BALL оставлен без изменений
- REC/AUDIO диктофон и WAV player

## Новое

LAB теперь содержит:

- WLAN `<NET>`
- BLE `<RADIO>`
- BALL `<IMU>`
- REC `<AUDIO>`
- TOOLS `<HW>`

TOOLS содержит:

- I2C Scanner на безопасной Grove-шине G1/G2
- SPI Test на EXT SPI: SCK G40, MOSI G14, MISO G39, CS G5
- UART Console: RX G13, TX G15, 3.3V TTL only
- GPIO Monitor: G1/G2 only
- PWM Generator: G1/G2 only
- DAC / PWM-DAC: честный PWM-DAC, не нативный DAC
- Logic Probe: G1/G2 only
- OLED Test: SSD1306 I2C на G1/G2
- Display Test: внутренний LCD
- RTC / Time: soft uptime/build time

## Не добавлено по решению проекта

- ADC Monitor
- I2C Sniffer

## Железная политика безопасности

Добавлены:

- `pin_registry.*` — whitelist и классификация пинов
- `bus_manager.*` — захват шин и конфликты
- `hardware_tools.*` — безопасный UI/логика инженерных инструментов

Основные правила:

- G8/G9 заблокированы как системная I2C: keyboard + IMU + audio codec control
- G10 заблокирован как battery ADC
- G11 заблокирован как keyboard INT
- G12/G14/G39/G40 — SD/SPI shared, использовать осторожно
- G33-G38 заблокированы как LCD
- G41/G42/G43/G46 заблокированы как audio I2S
- G44 заблокирован как IR TX
- G1/G2 — основной безопасный порт инструментов
- G13/G15 — UART console
- GPIO logic: 3.3V only, не подавать 5V

## Примечание

Я не прогонял реальную PlatformIO-сборку в этой среде, потому что здесь нет библиотек M5Cardputer/ESP32 Arduino. После первой сборки на MacBook/PlatformIO возможна точечная правка API M5Cardputer Audio или LEDC, если версия Arduino core отличается.
