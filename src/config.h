#pragma once

#include "fx/type.h"
#include "storage.h"

enum class AppState {
    INITIALIZATION,
    NORMAL,
    CALIBRATION,
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
};

class AppConfig {
public:
    Storage<Config> &storage;
    Config &config;

    unsigned long state_change_time = 0;
    AppState state = AppState::INITIALIZATION;

    const PaletteEntry *palette;

    explicit AppConfig(Storage<Config> &storage);

    void changeState(AppState s);
    void load();
    void update();
};

