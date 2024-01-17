#pragma once

#include "fx/type.h"
#include "storage.h"

class NtpTime;

enum class AppState {
    INITIALIZATION,
    NORMAL,
    CALIBRATION,
};

struct __attribute ((packed)) NightModeConfig {
    bool enabled = false;

    byte brightness = 10;
    byte eco = 20;

    uint32_t startTime = 0;
    uint32_t endTime = (uint32_t) 10 * 60 * 60;
    uint16_t switchInterval = (uint32_t) 15 * 60;
};

struct __attribute__ ((packed)) Config {
    bool power = true;

    byte maxBrightness = 128;

    byte speed = 180;
    byte scale = 64;
    byte light = 180;
    byte eco = 255;

    PaletteEnum palette = PaletteEnum::SUNSET;
    ColorEffectEnum colorEffect = ColorEffectEnum::GRADIENT;
    BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::DOUBLE_WAVE;

    uint32_t colorCorrection = TypicalLEDStrip;
    NightModeConfig nightMode;
};



