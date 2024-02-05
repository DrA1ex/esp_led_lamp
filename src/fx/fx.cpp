#include "fx.h"

#include "palette.h"

ColorEffectManager ColorEffects;
BrightnessEffectManager BrightnessEffects;

PaletteConfig Palettes = {
        .count = (uint8_t) palette_init.size(),
        .entries = palette_init
};