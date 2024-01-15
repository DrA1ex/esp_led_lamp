#pragma once

#include "led.h"

struct ColorEffectState {
    struct {
        const CRGBPalette16 *palette = nullptr;
        byte scale = 0;
        byte speed = 0;
    } params;

    unsigned long time = 0;
    unsigned long prev_time = 0;

    [[nodiscard]] inline unsigned long delta() const { return time - prev_time; }
};

typedef void(*ColorEffectFn)(Led &led, ColorEffectState &state);
typedef void (*BrightnessEffectFn)(Led &led, byte level);

enum class PaletteEnum : uint8_t {
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
    ECO,
};


template<typename C, typename V>
struct FxConfigEntry {
    static_assert(std::is_enum_v<C> && std::is_same_v<std::underlying_type_t<C>, uint8_t>);

    C code;
    const char *name;
    V value;
};

template<typename E>
struct FxConfig {
    uint8_t count = 0;
    std::vector<E> entries;
};

typedef FxConfigEntry<PaletteEnum, CRGBPalette16> PaletteEntry;
typedef FxConfig<PaletteEntry> PaletteConfig;

typedef FxConfigEntry<ColorEffectEnum, ColorEffectFn> ColorEffectEntry;
typedef FxConfig<ColorEffectEntry> ColorEffectConfig;

typedef FxConfigEntry<BrightnessEffectEnum, BrightnessEffectFn> BrightnessEffectEntry;
typedef FxConfig<BrightnessEffectEntry> BrightnessEffectConfig;