#pragma once

#include "fx/type.h"
#include "storage.h"

struct __attribute__ ((packed)) Config {
    bool power = true;

    byte maxBrightness = 128;

    byte speed = 180;
    byte scale = 64;
    byte light = 180;

    PaletteEnum palette = PaletteEnum::SUNSET;
    ColorEffectEnum colorEffect = ColorEffectEnum::GRADIENT;
    BrightnessEffectEnum brightnessEffect = BrightnessEffectEnum::DOUBLE_WAVE;
};

class AppConfig {
public:
    Storage<Config> &storage;
    volatile Config &config;

    const ColorEffectEntry *colorEffect;
    const BrightnessEffectEntry *brightnessEffect;
    const PaletteEntry *palette;

    explicit AppConfig(Storage<Config> &storage);

    void load();
    void update();
};

