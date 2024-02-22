#pragma once

#include "cctype"

enum class PaletteEnum : uint8_t {
    ABYSS,
    AQUATIC,
    AUTUMN,
    AZURE,
    BLOOM,
    BLOSSOM,
    BLUE,
    BONFIRE,
    CHERRY,
    CINNAMON,
    COASTAL,
    COSMIC,
    CRIMSON,
    DAWN,
    DEEP_SEA,
    DEEP,
    DREAMS,
    DUSK,
    EARTHY,
    ELECTRIC,
    ELEGANCE,
    EMERALD,
    ENCHANTED,
    ENIGMA,
    ETHEREAL,
    FIERY,
    FLAMES,
    FOREST,
    GOLDEN,
    HARMONY,
    HUSH,
    INFUSION,
    JADE,
    LAVENDER,
    LIMEADE,
    LUMINOUS,
    MAGIC_PLUM,
    MARINE,
    MEADOW,
    MIDNIGHT,
    MINTY,
    MONOCHROME,
    MOORLAND,
    MOSSY,
    MYSTIC,
    NEON,
    NIGHTFALL,
    NOCTURNAL,
    OASIS,
    OCEAN,
    OCEANIC,
    PASTEL,
    RAINBOW_SHADOW,
    RAINBOW,
    RASPBERRY,
    SCARLET,
    SEASIDE,
    SERENADE,
    SERENE,
    SERENITY,
    SLATE,
    SPICE,
    SPLASH,
    STARLIGHT,
    SULTRY,
    SUNNY,
    SUNRISE,
    SUNSET,
    TENDER,
    TERRAIN,
    TRANQUIL,
    TROPICS,
    TWILIGHT,
    URBAN,
    VELVET,
    VINTAGE,
    VIOLET,
    WOODLAND,

    CUSTOM,
};

enum class ColorEffectEnum : uint8_t {
    PERLIN,
    GRADIENT,
    FIRE,
    AURORA,
    PLASMA,
    RIPPLE,
    VELUM,
    PARTICLES,
    CHANGE_COLOR,
    SOLID,
};

enum class BrightnessEffectEnum : uint8_t {
    FIXED,
    PULSE,
    WAVE,
    DOUBLE_WAVE,
    OSCILLATOR,
};

enum class AudioEffectEnum : uint8_t {
    // Wave mode

    WAVE,
    WAVE_CENTERED,

    // Spectrum mode

    SPECTRUM,
    SPECTRUM_CENTERED,

    // Parametric mode
    BRIGHTNESS,

    LINE,
    LINE_CENTERED,

    SPEED_CONTROL,
    SCALE_CONTROL,
    LIGHT_CONTROL,
};