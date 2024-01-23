#pragma once

#include <cctype>

#include "misc/led.h"
#include "fx/type.h"

class NtpTime;

enum class AppState {
    INITIALIZATION,
    NORMAL,
    CALIBRATION,
};

struct __attribute ((packed)) NightModeConfig {
    bool enabled = false;

    uint8_t brightness = 10;
    uint8_t eco = 20;

    uint32_t start_time = 0;
    uint32_t end_time = (uint32_t) 10 * 60 * 60;
    uint16_t switch_interval = (uint32_t) 15 * 60;
};

struct __attribute ((packed)) Config {
    bool power = true;

    uint8_t max_brightness = 128;
    uint8_t eco = 255;

    uint8_t preset_id = 0;

    uint32_t color_correction = TypicalLEDStrip;
    NightModeConfig night_mode;
};

template<uint8_t N, uint8_t S>
struct __attribute ((packed)) __PresetNames {
    uint8_t count = N;
    uint8_t length = S;

    char names[N][S] = {};

    __PresetNames() {
        for (int i = 0; i < N; ++i) {
            sprintf(names[i], "Preset %i", i + 1);
        }
    }
};

struct __attribute ((packed)) PresetConfig {
    uint8_t speed = 180;
    uint8_t scale = 64;
    uint8_t light = 180;

    PaletteEnum palette = PaletteEnum::SUNSET;
    ColorEffectEnum color_effect = ColorEffectEnum::GRADIENT;
    BrightnessEffectEnum brightness_effect = BrightnessEffectEnum::DOUBLE_WAVE;
};

template<uint8_t N>
struct __attribute ((packed)) __PresetConfigs {
    PresetConfig presets[N] = {};
};

typedef __PresetNames<PRESET_MAX_COUNT, PRESET_NAME_MAX_SIZE + 1> PresetNames;
typedef __PresetConfigs<PRESET_MAX_COUNT> PresetConfigs;