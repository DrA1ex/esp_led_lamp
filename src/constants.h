#pragma once

#include "misc/led.h"
#include "sys_constants.h"

#define WIFI_MODE                               (WIFI_AP_MODE)
#define WIFI_SSID                               "ESP_LED"
#define WIFI_PASSWORD                           "12345678"

#define WIFI_CONNECTION_CHECK_INTERVAL          (5000u)                 // Interval (ms) between Wi-Fi connection check
#define WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL    (0u)                    // Max time (ms) to wait for Wi-Fi connection before switch to AP mode
                                                                        // 0 - Newer switch to AP mode

#define TIME_ZONE                               (5.f)                   // GMT +5:00

#define MDNS_NAME                               "esp_lamp"

#define BUTTON                                  DISABLED
#define BUTTON_PIN                              (4u)

#define LED_PIN                                 (5u)
#define LED_TYPE                                WS2812B
#define LED_COLOR_ORDER                         GRB

#define MATRIX_VOLTAGE                          (5u)
#define CURRENT_LIMIT                           (500u)

#define WIDTH                                   (16u)
#define HEIGHT                                  (16u)

#define CALIBRATION_TIMEOUT                     (5000u)                 // Timeout for calibration mode
#define POWER_CHANGE_TIMEOUT                    (1000u)                 // Timeout for power change animation

#define GAMMA_CORRECTION_RT                     DISABLED                // Real-time gamma correction.
                                                                        // Warning: rt-correction may have significant performance impact
                                                                        // when disabled - gamma correction will be applied to palette


#define AUDIO                                   DISABLED
#define AUDIO_PIN                               (0u)
#define AUDIO_GAIN                              (1u)
#define AUDIO_GATE                              (1u)
#define AUDIO_WINDOW_DURATION                   (5000u)
