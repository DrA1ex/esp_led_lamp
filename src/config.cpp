#include "config.h"

#include "fx/fx.h"

AppConfig::AppConfig(Config &config) : config(config) {
    colorEffectFn = ColorEffects[config.colorEffect];
    brightnessEffectFn = BrightnessEffects[config.brightnessEffect];
    palette = &Palettes[config.palette];
}
