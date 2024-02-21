#pragma once

#define DISABLED                                0
#define ENABLED                                 1

#define WIFI_AP_MODE                            0
#define WIFI_STA_MODE                           1

#define FRAMES_PER_SECOND                       (60u)

#define UDP_PORT                                (1423)
#define UDP_MAX_PACKET_SIZE                     (260u)

#define WS_MAX_PACKET_SIZE                      (260u)
#define WS_MAX_PACKET_QUEUE                     (10u)

#define PACKET_SIGNATURE                        ((uint16_t) 0xAA34)

#define WEB_PORT                                (80)

#define STORAGE_PATH                            ("/__storage/")
#define STORAGE_HEADER                          ((uint32_t) 0xffaabbcc)
#define STORAGE_CONFIG_VERSION                  ((uint8_t) 9)
#define STORAGE_PRESET_NAMES_VERSION            ((uint8_t) 4)
#define STORAGE_PRESET_CONFIG_VERSION           ((uint8_t) 5)
#define STORAGE_CUSTOM_PALETTE_VERSION          ((uint8_t) 2)
#define STORAGE_SAVE_INTERVAL                   (60000u)                // Wait before commit settings to FLASH

#define TIMER_GROW_AMOUNT                       (8u);

#define MAX_PARTICLES_COUNT                     (100u)

#define MAX_SAFE_FLOAT_VALUE                    (9007199254740991LL)    // 2 ** 53

#define PRESET_MAX_COUNT                        (24u)
#define PRESET_NAME_MAX_SIZE                    (32u)                   // Max size in bytes, UTF-8

#define BTN_SILENCE_INTERVAL                    (40u)
#define BTN_HOLD_INTERVAL                       (500u)
#define BTN_PRESS_WAIT_INTERVAL                 (500u)
#define BTN_RESET_INTERVAL                      (1000u)


#define AUDIO_SAMPLE_RATE                       (9400u)
#define AUDIO_MAX_SIGNAL                        (1024u)