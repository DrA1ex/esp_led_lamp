#pragma once

#include "fx/type.h"

struct Config {
    byte maxBrightness = 128;

    byte speed = 180;
    byte scale = 64;
    byte light = 180;

    PaletteEnum palette = PaletteEnum::SUNSET;
    ColorEffectEnum colorEffect = ColorEffectEnum::PERLIN;
    BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::DOUBLE_WAVE;
};

struct AppConfig {
    Config &config;

    ColorEffectFn colorEffectFn;
    BrightnessEffectFn brightnessEffectFn;
    const CRGBPalette16 *palette;

    explicit AppConfig(Config &config);

    void update();
};

