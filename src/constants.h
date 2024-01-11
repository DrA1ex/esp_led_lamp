#pragma once

#include <FastLED.h>

#define WIFI_MODE                               (0u)
#define WIFI_SSID                               "ESP_LED"
#define WIFI_PASSWORD                           "12345678"

#define WIFI_CONNECTION_CHECK_INTERVAL          (5000u)
#define WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL    (0u)

#define UDP_PORT                                (1423)
#define UDP_MAX_PACKET_SIZE                     (260u)
#define UDP_PACKET_SIGNATURE                    ((uint16_t) 0xAABB1234)

#define LED_PIN                                 (5u)
#define LED_TYPE                                WS2812B
#define LED_COLOR_ORDER                         GRB

#define MATRIX_VOLTAGE                          (5u)
#define CURRENT_LIMIT                           (500u)

#define WIDTH                                   (34u)
#define HEIGHT                                  (1u)

#define FRAMES_PER_SECOND                       (60u)