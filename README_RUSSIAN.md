# ESP LED LAMP

Атмосферное освещение для светодиодных лент и матриц.

<p align="center">
  <img alt="Animation" height="256" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/c33b63fe-1197-439c-8b91-698c3b74b915">
</p>

## UI

<p align="center">
  <img alt="UI Light" height="480" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/841e800b-41cc-4933-9e5f-deb624b7f1e0">
  <img alt="UI Dark" height="480" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/9d3c6928-69de-4096-9cef-5e063a872605">
</p>

### Инструкция по установке PWA (Веб как приложение)
Для Android:
1. Откройте Chrome.
2. Перейдите на [http://esp_lamp.local](http://esp_lamp.local).
3. Нажмите "Добавить на главный экран."

Для iOS:
1. Откройте Safari.
2. Перейдите на [http://esp_lamp.local](http://esp_lamp.local).
3. Нажмите кнопку "Поделиться".
4. Выберите "Добавить на главный экран."

## Настройка

Для разных устройств используйте пресеты, не редактируя исходные файлы.

1. Скопируйте [presets/credentials.example.ini](presets/credentials.example.ini) в
   `presets/credentials.ini` и укажите общие реквизиты Wi-Fi, веб-интерфейса и MQTT.
2. Скопируйте [presets/device.example.ini](presets/device.example.ini), например,
   в `presets/ceiling.ini` и задайте параметры устройства.
3. При сборке или прошивке указывайте `--preset ceiling`, см. команды ниже.

Личные `presets/*.ini` исключены из Git; в репозитории хранятся только шаблоны
`*.example.ini`. Не записывайте настоящие пароли в шаблоны.
Старые файлы `src/_override/*.h` больше не подключаются.

```ini
[firmware]
WIFI_MODE = WIFI_STA_MODE
MDNS_NAME = "ceiling_lamp"
WIDTH = 153
HEIGHT = 1
LED_COLOR_ORDER = RGB
CURRENT_LIMIT = 2000
MQTT = true
WEB_AUTH = true

[upload]
# Необязательно: иначе OTA использует ceiling_lamp.local.
# ota_host = "192.168.1.42"
# Необязательно: иначе PlatformIO определит USB-порт автоматически.
# usb_port = "COM3"
```

Формат: `ИМЯ = значение`. Строки — в двойных кавычках, числа, `true` / `false`
и символы прошивки (`WIFI_STA_MODE`, `DISABLED`, `WS2812B`, `RGB`) — без кавычек.
В строках экранирование как в JSON: `\"` для кавычки и `\\` для обратного слеша;
пробелы, `#`, `;` и `%` сохраняются. Комментарии начинайте с `#` или `;` на
отдельной строке, не после значения. Регистр имён важен, неизвестные параметры
останавливают сборку. `WEB_AUTH = false` отключает веб-аутентификацию.

Приоритет настроек: значения по умолчанию из `src/constants.h`, `src/credentials.h`
и `src/sys_constants.h` → общие `presets/credentials.ini` (если файл существует)
→ выбранный INI устройства. В пресете можно переопределить и общие реквизиты.
Для другого набора общих реквизитов используйте `--credentials presets/work.ini`.
Имена параметров `[firmware]` соответствуют существующим `#define` в этих трёх
заголовках. Без пресета PlatformIO использует значения по умолчанию и общий файл
реквизитов, если он есть. Все пресеты одного окружения используют общий каталог
`.pio/build/<окружение>/`. При изменении заголовка настроек пересобираются зависимые
исходники, а неизменившиеся библиотеки и объекты Arduino framework используются
повторно. Файл прошивки в этом каталоге соответствует последнему собранному
пресету. Скрипт по умолчанию использует `release` для USB и OTA; у `debug` и
`local-audio` остаются отдельные каталоги.

В таблицах ниже приведены значения по умолчанию, которые можно менять через `[firmware]`.

| Параметр           | Описание                                    | Значение по умолчанию |
|---------------------|---------------------------------------------|-----------------------|
| `WIFI_SSID`         | SSID сети Wi-Fi, к которой нужно подключиться | "ESP_LED"             |
| `WIFI_PASSWORD`     | Пароль для сети Wi-Fi                       | "12345678"            |
| `AUTH_USER`         | Имя пользователя для веб-аутентификации    | "esp_lamp"            |
| `AUTH_PASSWORD`     | Пароль для веб-аутентификации               | "password"            |
| `MQTT_HOST`         | Имя хоста или IP-адрес для MQTT брокера    | "example.com"         |
| `MQTT_PORT`         | Номер порта для MQTT брокера                | 1234                  |
| `MQTT_USER`         | Имя пользователя для MQTT аутентификации    | "esp_user"            |
| `MQTT_PASSWORD`     | Пароль для MQTT аутентификации              | "esp_pass"            |

Параметры оборудования и приложения по умолчанию заданы в [src/constants.h](src/constants.h).

| Параметр                             | Описание                                                        | Значение по умолчанию |
|---------------------------------------|-----------------------------------------------------------------|-----------------------|
| WIFI_MODE                             | Режим работы Wi-Fi (например, AP-режим или режим станции)    | `WIFI_AP_MODE`        |
| WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL  | Максимальное время (мс) ожидания подключения к Wi-Fi перед переключением в режим точки доступа. Значение 0 означает, что он никогда не переключается. | `0`                   |
| WEB_AUTH                              | Включает функции веб-аутентификации                             | Включено              |
| TIME_ZONE                             | Смещение часового пояса от GMT (например, GMT +5:00)            | `5.f`                 |
| MDNS_NAME                             | Имя mDNS для устройства (например, `esp_lamp`.local) | `"esp_lamp"`         |
| BUTTON                                | Указывает, включена ли функция кнопки.                          | `DISABLED`            |
| BUTTON_PIN                            | Номер пина GPIO для кнопки.                                    | `4u`                  |
| LED_PIN                               | Номер пина GPIO для светодиодного выхода.                      | `5u`                  |
| LED_TYPE                              | Тип используемого светодиода (например, WS2812B).              | `WS2812B`             |
| LED_COLOR_ORDER                       | Порядок цветов для светодиода (например, RGB, GRB формат).     | `GRB`                 |
| CURRENT_LIMIT                         | Максимальный предел тока для устройства в миллиамперах.       | `500u`                |
| WIDTH                                 | Количество светодиодов матрицы или ленты по ширине                        | `16u`                 |
| HEIGHT                                | Количество светодиодов по высоте                         | `16u`                 |
| AUDIO                                 | Включает аудиофункции.                                        | `DISABLED`            |
| AUDIO_PIN                             | Номер аналогового пина, назначенный для аудиовыхода.                  | `0u`                  |
| MQTT                                  | Включает протокол MQTT.                              | `0u`                  |

## MQTT Топики

| Топик Вход*              | Топик Выход*               | Тип            | Значения             | Комментарии                              |
|--------------------------|-----------------------------|----------------|----------------------|------------------------------------------|
| MQTT_TOPIC_POWER         | MQTT_OUT_TOPIC_POWER        | uint8_t        | 0..1                 | Состояние питания: ВКЛ (1) / ВЫКЛ (0)    |
| MQTT_TOPIC_BRIGHTNESS    | MQTT_OUT_TOPIC_BRIGHTNESS   | uint8_t        | 0..255               | Уровень яркости                         |
| MQTT_TOPIC_SPEED         | MQTT_OUT_TOPIC_SPEED        | uint8_t        | 0..255               | Параметр скорости цветового эффекта     |
| MQTT_TOPIC_SCALE         | MQTT_OUT_TOPIC_SCALE        | uint8_t        | 0..255               | Параметр масштаба цветового эффекта     |
| MQTT_TOPIC_LIGHT         | MQTT_OUT_TOPIC_LIGHT        | uint8_t        | 0..255               | Параметр света эффекта яркости          |
| MQTT_TOPIC_PRESET        | MQTT_OUT_TOPIC_PRESET       | uint8_t        | 0..23                | Выбор пресета                     |
| MQTT_TOPIC_PALETTE       | MQTT_OUT_TOPIC_PALETTE      | uint8_t        | 0..78                | Выбор палитры                           |
| MQTT_TOPIC_COLOR         | MQTT_OUT_TOPIC_COLOR        | uint32_t       | RGB (например, 0xff00ff) | RGB цвет цветового режима                |
| MQTT_TOPIC_NIGHT_MODE    | MQTT_OUT_TOPIC_NIGHT_MODE   | uint8_t        | 0..1                 | Состояние ночного режима: ВКЛ (1) / ВЫКЛ (0) |

* Фактические значения тем объявлены в constants.h

## Установка Прошивки

### Предварительные условия

Убедитесь, что у вас установлено:

1. [PlatformIO](https://platformio.org/?utm_source=platformio&utm_medium=piohome)
2. [Node.js](https://nodejs.org)

### Загрузка Прошивки

Выполняйте команды из корня репозитория. Python-скрипт работает в Windows, macOS
и Linux без Bash и дополнительных Python-пакетов. Команда PlatformIO `pio`
должна быть в `PATH`. Используйте доступную у вас команду Python 3: `python`,
`python3` или `py -3` в Windows.

```sh
# Только собрать, без прошивки устройства.
python tools/firmware.py --preset ceiling

# Собрать и прошить через USB/UART с автоматическим определением порта.
python tools/firmware.py --preset ceiling --upload usb

# Явно указать последовательный порт (пример для Windows).
python tools/firmware.py --preset ceiling --upload usb --port COM3

# Собрать и прошить по Wi-Fi: адрес из MDNS_NAME.local или [upload] ota_host.
python tools/firmware.py --preset ceiling --upload ota

# Разово переопределить адрес назначения.
python tools/firmware.py --preset ceiling --upload ota --port 192.168.1.42

# Другой общий файл реквизитов или окружение сборки.
python tools/firmware.py --preset ceiling --credentials presets/work.ini --environment debug
```

### Обновление нескольких устройств

```sh
python tools/firmware.py --preset all --upload ota --dry-run
python tools/firmware.py --preset all --upload ota
python tools/firmware.py --preset all --upload ota --exclude test
```

`all` выбирает пресеты устройств с `MDNS_NAME` или `ota_host`.
Чтобы исключить устройство из массового обновления, добавьте
`batch = false` в секцию `[upload]` его пресета. При ошибке скрипт
переходит к следующей лампе; `--fail-fast` останавливает процесс.
Для подробного вывода используйте `--verbose` (логи в `.pio/logs/`).

Для одной лампы укажите `--preset NAME`, для изменения адреса — `--port HOST`.
OTA требует доступной лампы; первая установка прошивки выполняется через USB.

### Обновление веб-интерфейса (LittleFS)

Чтобы отдельно собрать и сжать веб-файлы:

```sh
python tools/web_bundle.py
```

Или собрать их автоматически и обновить одну лампу либо все устройства:

```sh
python tools/firmware.py --preset ceiling --filesystem --upload ota --build-web
python tools/firmware.py --preset all --filesystem --upload ota --build-web
```

Без `--build-web` используются готовые файлы из `data/`.
Настройки каждой лампы сохраняются в её образе. Недоступные устройства
пропускаются, копии остаются в `.pio/storage-backups/`.

Для первоначальной установки файловой системы:

```sh
python tools/firmware.py --preset ceiling --filesystem --upload usb --initialize --build-web
```

Флаг `--initialize` обязателен при загрузке LittleFS через USB и недоступен
для `--preset all`. Скрипт `upload_fs.sh` тоже собирает веб-файлы
автоматически (`OTA=1 LAMP_PRESET=ceiling ./upload_fs.sh`).
