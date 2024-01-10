#pragma once

#include "fx/type.h"

struct Config {
    byte maxBrightness = 255;

    byte speed = 255;
    byte scale = 64;
    byte light = 180;

    PaletteEnum palette = PaletteEnum::OCEAN;
    ColorEffectEnum colorEffect = ColorEffectEnum::PACIFIC;
    BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::FIXED;
};

struct AppConfig {
    Config &config;

    ColorEffectFn colorEffectFn;
    BrightnessEffectFn brightnessEffectFn;
    const CRGBPalette16 *palette;

    explicit AppConfig(Config &config);
};

