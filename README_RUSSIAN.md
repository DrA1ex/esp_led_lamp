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

Настройте данные аутентификации, отредактировав файл [src/credentials.h](src/credentials.h).

| Параметр           | Описание                                    | Значение по умолчанию |
|---------------------|---------------------------------------------|-----------------------|
| `WIFI_SSID`         | SSID сети Wi-Fi, к которой нужно подключиться | "ESP_LED"             |
| `WIFI_PASSWORD`     | Пароль для сети Wi-Fi                       | "12345678"            |
| `WEBAUTH_USER`      | Имя пользователя для веб-аутентификации    | "esp_led"             |
| `WEBAUTH_PASSWORD`  | Пароль для веб-аутентификации               | "password"            |
| `MQTT_HOST`         | Имя хоста или IP-адрес для MQTT брокера    | "example.com"         |
| `MQTT_PORT`         | Номер порта для MQTT брокера                | 1234                  |
| `MQTT_USER`         | Имя пользователя для MQTT аутентификации    | "esp_user"            |
| `MQTT_PASSWORD`     | Пароль для MQTT аутентификации              | "esp_pass"            |

Настройте приложение под ваши специфические параметры, отредактировав файл [src/constants.h](src/constants.h).

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

Для загрузки прошивки выполните следующие команды:

- Для загрузки через USB:
```sh
pio run -e release
```

- Для OTA загрузки с адресом по умолчанию (`esp_lamp.local`):
```sh
pio run -e ota -t upload
```

- Для OTA загрузки с кастомным адресом:
```sh
pio run -e ota -t upload --upload-port esp_lamp_2.local
```

### Загрузка Файловой Системы

Чтобы загрузить файловую систему, выполните следующие шаги:

1. Сделайте скрипт upload_fs.sh исполняемым:
```sh
chmod +x ./upload_fs.sh
```

- Для автоматической сборки WebUI и загрузки его с помощью OTA с адресом по умолчанию (MacOS, *nix):
```sh
OTA=1 ./upload_fs.sh
```

- Для загрузки OTA с пользовательским адресом:
```sh
OTA=1 ./upload_fs.sh --upload-port esp_lamp_2.local
```

- Для пользователей Windows используйте ручную сборку:

1. Соберите WebUI:
```sh
cd ./www
npm run build
cd ..
```

2. Загрузите файловую систему с помощью OTA:
```sh
pio run -t uploadfs -e ota
```
