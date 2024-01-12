#pragma once

#include "fx/type.h"

struct __attribute__ ((packed)) Config {
    bool power = true;

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

    const ColorEffectEntry *colorEffect;
    const BrightnessEffectEntry *brightnessEffect;
    const PaletteEntry *palette;

    explicit AppConfig(Config &config);

    void update();
};

