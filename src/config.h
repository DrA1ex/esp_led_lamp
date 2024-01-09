#pragma once

#include "fx/type.h"

struct Config {
    byte maxBrightness = 255;

    byte speed = 200;
    byte scale = 64;
    byte brightness = 200;

    PaletteEnum palette = PaletteEnum::RAINBOW;
    ColorEffectEnum colorEffect = ColorEffectEnum::GRADIENT;
    BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::WAVE;
};

struct AppConfig {
    Config &config;

    ColorEffectFn colorEffectFn;
    BrightnessEffectFn brightnessEffectFn;
    const CRGBPalette16 *palette;

    explicit AppConfig(Config &config);
};

