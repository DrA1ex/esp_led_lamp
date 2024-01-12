#include "fx.h"

#include "brightness_effect.h"
#include "color_effect.h"
#include "palette.h"

PaletteConfig Palettes = {
        .count = 32,
        .entries = {
                {PaletteEnum::CUSTOM,         "Custom",         CustomPal},
                {PaletteEnum::HEAT_COLORS,    "Heat",           HeatColors_p},
                {PaletteEnum::FIRE,           "Fire",           Fire_gp},
                {PaletteEnum::WOOD_FIRE,      "Wood Fire",      WoodFireColors_p},
                {PaletteEnum::NORMAL_FIRE,    "Normal Fire",    NormalFire_p},
                {PaletteEnum::LITHIUM_FIRE,   "Lithium Fire",   LithiumFireColors_p},
                {PaletteEnum::SODIUM_FIRE,    "Sodium Fire",    SodiumFireColors_p},
                {PaletteEnum::COPPER_FIRE,    "Copper Fire",    CopperFireColors_p},
                {PaletteEnum::ALCOHOL_FIRE,   "Alcohol Fire",   AlcoholFireColors_p},
                {PaletteEnum::LAVA,           "Lava",           LavaColors_p},
                {PaletteEnum::PARTY,          "Party",          PartyColors_p},
                {PaletteEnum::RAINBOW,        "Rainbow",        RainbowColors_p},
                {PaletteEnum::RAINBOW_STRIPE, "Rainbow Stripe", RainbowStripeColors_p},
                {PaletteEnum::CLOUD,          "Cloud",          CloudColors_p},
                {PaletteEnum::OCEAN,          "Ocean",          OceanColors_p},
                {PaletteEnum::FOREST,         "Forest",         ForestColors_p},
                {PaletteEnum::SUNSET,         "Sunset",         Sunset_Real_gp},
                {PaletteEnum::DK_BLUE_RED,    "Blue-Red",       Dkbluered_gp},
                {PaletteEnum::OPTIMUS_PRIME,  "Optimus Prime",  Optimus_Prime_gp},
                {PaletteEnum::WARM,           "Warm",           WarmGrad_gp},
                {PaletteEnum::COLD,           "Cold",           ColdGrad_gp},
                {PaletteEnum::HOT,            "Hot",            HotGrad_gp},
                {PaletteEnum::PINK,           "Pink",           PinkGrad_gp},
                {PaletteEnum::COMFY,          "Comfy",          Comfy_gp},
                {PaletteEnum::CYBERPUNK,      "Cyberpunk",      Cyberpunk_gp},
                {PaletteEnum::GIRL,           "Girl",           Girl_gp},
                {PaletteEnum::XMAS,           "Xmas",           Xmas_gp},
                {PaletteEnum::ACID,           "Acid",           Acid_gp},
                {PaletteEnum::BLUE_SMOKE,     "Blue Smoke",     BlueSmoke_gp},
                {PaletteEnum::GUMMY,          "Gummy",          Gummy_gp},
                {PaletteEnum::LEO,            "Leo",            Leo_gp},
                {PaletteEnum::AURORA,         "Aurora",         Aurora_gp},
        }
};

ColorEffectConfig ColorEffects = {
        .count = 6,
        .entries = {
                {ColorEffectEnum::PERLIN, "Perlin Noise", perlin},
                {ColorEffectEnum::GRADIENT, "Gradient", gradient},
                {ColorEffectEnum::PACIFIC, "Pacific", pacific},
                {ColorEffectEnum::PARTICLES, "Particles", particles},
                {ColorEffectEnum::CHANGE_COLOR, "Color Change", changeColor},
                {ColorEffectEnum::SOLID, "Solid Color", solid},
        }
};

BrightnessEffectConfig BrightnessEffects = {
        .count = 4,
        .entries = {
                {BrightnessEffectEnum::FIXED, "Fixed", fixed},
                {BrightnessEffectEnum::PULSE, "Pulse", pulse},
                {BrightnessEffectEnum::WAVE, "Wave", wave},
                {BrightnessEffectEnum::DOUBLE_WAVE, "Double Wave", double_wave}
        }
};