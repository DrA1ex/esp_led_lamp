#pragma once

#include <cctype>

#include "misc/led.h"
#include "fx/type.h"

class NtpTime;

enum class AppState: uint8_t {
    INITIALIZATION,
    NORMAL,
    CALIBRATION,
    TURNING_ON,
    TURNING_OFF
};

struct BrightnessSettings {
    uint8_t brightness;
    uint8_t eco;
};

struct __attribute ((packed)) NightModeConfig {
    bool enabled = false;

    uint8_t brightness = 10;
    uint8_t eco = 20;

    uint32_t start_time = 0;
    uint32_t end_time = (uint32_t) 10 * 60 * 60;
    uint16_t switch_interval = (uint32_t) 15 * 60;
};

struct __attribute ((packed)) AudioConfig {
    bool enabled = false;

    uint8_t gain = 0;
    uint8_t gate = 0;

    AudioEffectEnum effect = AudioEffectEnum::WAVE_CENTERED;

    uint8_t min = 0;
    uint8_t max = 255;

    [[nodiscard]] inline bool is_wave() const { return enabled && effect >= AudioEffectEnum::WAVE && effect < AudioEffectEnum::SPECTRUM; }
    [[nodiscard]] inline bool is_spectrum() const {return enabled && effect >= AudioEffectEnum::SPECTRUM && effect < AudioEffectEnum::BRIGHTNESS;}
    [[nodiscard]] inline bool is_parametric() const { return enabled && effect >= AudioEffectEnum::BRIGHTNESS; }
};

struct __attribute ((packed)) Config {
    bool power = true;

    uint8_t max_brightness = 128;
    uint8_t eco = 255;

    uint8_t preset_id = 0;

    uint32_t color_correction = TypicalLEDStrip;
    NightModeConfig night_mode;

    uint8_t gamma = 128; // 2.2

    AudioConfig audio_config;
};

template<uint8_t N, uint8_t S>
struct __attribute ((packed)) __PresetNames {
    uint8_t count = N;
    uint8_t length = S;

    char names[N][S] = {};
    bool custom[N] = {};

    __PresetNames() {
        memset(names, 0, sizeof(names));
        memset(custom, 0, sizeof(custom));
    }
};

struct __attribute ((packed)) PresetConfig {
    uint8_t speed = 80;
    uint8_t scale = 190;
    uint8_t light = 128;

    PaletteEnum palette;
    ColorEffectEnum color_effect;
    BrightnessEffectEnum brightness_effect;

    PresetConfig() {
        palette = (PaletteEnum) random((int) PaletteEnum::CUSTOM); // Exclude Custom
        color_effect = (ColorEffectEnum) random((int) ColorEffectEnum::CHANGE_COLOR); // Exclude CHANGE_COLOR and SOLID
        brightness_effect = BrightnessEffectEnum::FIXED;
    }
};

template<uint8_t N>
struct __attribute ((packed)) __PresetConfigs {
    uint8_t count = N;

    PresetConfig presets[N] = {};
};

typedef __PresetNames<PRESET_MAX_COUNT, PRESET_NAME_MAX_SIZE + 1> PresetNames;
typedef __PresetConfigs<PRESET_MAX_COUNT> PresetConfigs;

struct CustomPaletteConfig {
    bool updated = false;
    PaletteColorT colors[16];
};