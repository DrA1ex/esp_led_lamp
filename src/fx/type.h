#pragma once

#include "led.h"

typedef void(*ColorEffectFn)(Led &led, const CRGBPalette16 &palette, byte scale, byte speed);
typedef void (*BrightnessEffectFn)(Led &led, byte level);

enum PaletteEnum : uint8_t {
    CUSTOM,
    HEAT_COLORS,
    FIRE,
    WOOD_FIRE,
    NORMAL_FIRE,
    LITHIUM_FIRE,
    SODIUM_FIRE,
    COPPER_FIRE,
    ALCOHOL_FIRE,
    LAVA,
    PARTY,
    RAINBOW,
    RAINBOW_STRIPE,
    CLOUD,
    OCEAN,
    FOREST,
    SUNSET,
    DK_BLUE_RED,
    OPTIMUS_PRIME,
    WARM,
    COLD,
    HOT,
    PINK,
    COMFY,
    CYBERPUNK,
    GIRL,
    XMAS,
    ACID,
    BLUE_SMOKE,
    GUMMY,
    LEO,
    AURORA,
};

enum ColorEffectEnum : uint8_t {
    PERLIN,
    GRADIENT,
    PACIFIC,
    PARTICLES,
    CHANGE_COLOR,
    SOLID,
};

enum BrightnessEffectEnum : uint8_t {
    FIXED,
    PULSE,
    WAVE,
    DOUBLE_WAVE,
};