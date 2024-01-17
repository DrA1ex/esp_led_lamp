#pragma once

#include <cctype>
#include <FastLED.h>

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

    uint32_t startTime = 0;
    uint32_t endTime = (uint32_t) 10 * 60 * 60;
    uint16_t switchInterval = (uint32_t) 15 * 60;
};

struct __attribute__ ((packed)) Config {
    bool power = true;

    uint8_t maxBrightness = 128;

    uint8_t speed = 180;
    uint8_t scale = 64;
    uint8_t light = 180;
    uint8_t eco = 255;

    PaletteEnum palette = PaletteEnum::SUNSET;
    ColorEffectEnum colorEffect = ColorEffectEnum::GRADIENT;
    BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::DOUBLE_WAVE;

    uint32_t colorCorrection = TypicalLEDStrip;
    NightModeConfig nightMode;
};



