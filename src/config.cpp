#include "config.h"

#include "fx/fx.h"

AppConfig::AppConfig(Storage<Config> &storage) : storage(storage), config(storage.get()) {}

void AppConfig::load() {
    ColorEffects.select(config.colorEffect);

    if ((int) config.brightnessEffect < BrightnessEffects.count) {
        brightnessEffect = &BrightnessEffects.entries[(int) config.brightnessEffect];
    } else {
        brightnessEffect = &BrightnessEffects.entries[(int) BrightnessEffectEnum::FIXED];
    }

    if ((int) config.palette < Palettes.count) {
        palette = &Palettes.entries[(int) config.palette];
    } else {
        palette = &Palettes.entries[(int) PaletteEnum::RAINBOW];
    }
}

void AppConfig::update() {
    storage.save();
    load();
}

void AppConfig::changeState(AppState s) {
    state_change_time = millis();
    state = s;
}