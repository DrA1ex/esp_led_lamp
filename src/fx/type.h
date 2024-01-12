#pragma once

#include "led.h"

typedef void(*ColorEffectFn)(Led &led, const CRGBPalette16 &palette, byte scale, byte speed);
typedef void (*BrightnessEffectFn)(Led &led, byte level);

enum class PaletteEnum : uint8_t {
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

enum class ColorEffectEnum : uint8_t {
    PERLIN,
    GRADIENT,
    PACIFIC,
    PARTICLES,
    CHANGE_COLOR,
    SOLID,
};

enum class BrightnessEffectEnum : uint8_t {
    FIXED,
    PULSE,
    WAVE,
    DOUBLE_WAVE,
};


template<typename C, typename V>
struct __attribute__ ((packed)) ConfigEntry {
    C code;
    const char *name;
    V value;
};

typedef ConfigEntry<PaletteEnum, CRGBPalette16> PaletteEntry;
struct __attribute__ ((packed))PaletteConfig {
    uint8_t count = 0;
    PaletteEntry entries[];
};

typedef ConfigEntry<ColorEffectEnum, ColorEffectFn> ColorEffectEntry;
struct __attribute__ ((packed)) ColorEffectConfig {
    uint8_t count = 0;
    ColorEffectEntry entries[];
};

typedef ConfigEntry<BrightnessEffectEnum, BrightnessEffectFn> BrightnessEffectEntry;
struct __attribute__ ((packed)) BrightnessEffectConfig {
    uint8_t count = 0;
    BrightnessEffectEntry entries[];
};