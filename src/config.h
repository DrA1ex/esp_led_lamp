#pragma once

#include "fx/type.h"

struct Config {
    volatile byte maxBrightness = 255;

    volatile byte speed = 255;
    volatile byte scale = 64;
    volatile byte light = 180;

    volatile PaletteEnum palette = PaletteEnum::OCEAN;
    volatile ColorEffectEnum colorEffect = ColorEffectEnum::PACIFIC;
    volatile BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::FIXED;
};

struct AppConfig {
    Config &config;

    volatile ColorEffectFn colorEffectFn;
    volatile BrightnessEffectFn brightnessEffectFn;
    volatile const CRGBPalette16 *palette;

    explicit AppConfig(Config &config);

    void update();
};

