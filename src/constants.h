#pragma once

#include <FastLED.h>

#define WIFI_MODE                               (0u)                    // 0 - AP mode, 1 - STA mode
#define WIFI_SSID                               "ESP_LED"
#define WIFI_PASSWORD                           "12345678"

#define WIFI_CONNECTION_CHECK_INTERVAL          (5000u)                 // Interval (ms) between Wi-Fi connection check
#define WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL    (0u)                    // Max time (ms) to wait for Wi-Fi connection before switch to AP mode
                                                                        // 0 - Newer switch to AP mode

#define TIME_ZONE                               (5.f)                     // GMT +5:00

#define UDP_PORT                                (1423)
#define UDP_MAX_PACKET_SIZE                     (260u)

#define PACKET_SIGNATURE                        ((uint16_t) 0xAA34)

#define MDNS_NAME                               "esp_lamp"

#define WEB_PORT                                (80)

#define LED_PIN                                 (5u)
#define LED_TYPE                                WS2812B
#define LED_COLOR_ORDER                         GRB

#define MATRIX_VOLTAGE                          (5u)
#define CURRENT_LIMIT                           (500u)

#define WIDTH                                   (34u)
#define HEIGHT                                  (1u)

#define FRAMES_PER_SECOND                       (60u)

#define STORAGE_HEADER                          ((uint32_t) 0xffaabbcc)
#define STORAGE_VERSION                         ((uint8_t) 4)
#define STORAGE_SAVE_INTERVAL                   (60000u)                // Wait before commit settings to FLASH

#define TIMER_GROW_AMOUNT                       (8u);

#define CALIBRATION_TIMEOUT                     (5000u)                 // Timeout for calibration mode

#define MAX_PARTICLES_COUNT                     (50u)

#define MAX_SAFE_FLOAT_VALUE                    (4194304)               // 2 ** 22