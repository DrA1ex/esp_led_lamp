#include "fx.h"

#include "palette.h"

ColorEffectManager ColorEffects;
BrightnessEffectManager BrightnessEffects;

const std::initializer_list<PaletteEntry> palette_init = {
        {PaletteEnum::AQUA_BLOOM,          "Aqua Bloom",         AquaBloom_gp},
        {PaletteEnum::AQUA_MARINE,         "Aqua Marine",        AquaMarine_gp},
        {PaletteEnum::AUTUMN_FIRE,         "Autumn Fire",        AutumnFire_gp},
        {PaletteEnum::AZURE_WATERS,        "Azure Waters",       AzureWaters_gp},
        {PaletteEnum::BEACH_ELEGANCE,      "Beach Elegance",     BeachElegance_gp},
        {PaletteEnum::BRIGHT_SKY,          "Bright Sky",         BrightSky_gp},
        {PaletteEnum::BROWNISH_RED,        "Brownish Red",       BrownishRed_gp},
        {PaletteEnum::CALM_LAKE,           "Calm Lake",          CalmLake_gp},
        {PaletteEnum::CALM_OASIS,          "Calm Oasis",         CalmOasis_gp},
        {PaletteEnum::CAMP_FIRE,           "Camp Fire",          CampFire_gp},
        {PaletteEnum::COLORFUL_RAINBOW,    "Colorful Rainbow",   ColorfulRainbow_gp},
        {PaletteEnum::CRIMSON_RHAPSODY,    "Crimson Rhapsody",   CrimsonRhapsody_gp},
        {PaletteEnum::DARK_BLAZE,          "Dark Blaze",         DarkBlase_gp},
        {PaletteEnum::DARK_MYSTERY,        "Dark Mystery",       DarkMystery_gp},
        {PaletteEnum::ELECTRIC_BLOOM,      "Electric Bloom",     ElectricBloom_gp},
        {PaletteEnum::EMERALD_OASIS,       "Emerald Oasis",      EmeraldOasis_gp},
        {PaletteEnum::ENCHANTED_WOODS,     "Enchanted Woods",    EnchantedForest_gp},
        {PaletteEnum::ETHEREAL_EVOLUTION,  "Ethereal Evolution", EtherealEvolution_gp},
        {PaletteEnum::ETHEREAL_SERENADE,   "Ethereal Serenade",  EtherealSerenade_gp},
        {PaletteEnum::FIERY_BLEND,         "Fiery Blend",        FieryBlend_gp},
        {PaletteEnum::FIERY_FUSION,        "Fiery Fusion",       FieryFusion_gp},
        {PaletteEnum::FOREST_RETREAT,      "Forest Retreat",     ForestRetreat_gp},
        {PaletteEnum::GOLDEN_HORIZON,      "Golden Horizon",     GoldenHorizon_gp},
        {PaletteEnum::GOLDEN_SPICE,        "Golden Spice",       GoldenSpice_gp},
        {PaletteEnum::GRAY_HAVEN,          "Gray Haven",         GrayHeaven_gp},
        {PaletteEnum::GREEN_HARMONY,       "Green Harmony",      GrayHarmony_gp},
        {PaletteEnum::GREEN_OASIS,         "Green Oasis",        GreenOasis_gp},
        {PaletteEnum::GREEN_WOODS,         "Green Woods",        GreenWoods_gp},
        {PaletteEnum::LIMEADE,             "Limeade",            Limeade_gp},
        {PaletteEnum::LIME_SPLASH,         "Lime Splash",        LimeSplash_gp},
        {PaletteEnum::LUXURIOUS_VELVET,    "Luxurious Velvet",   LuxuriousVelvet_gp},
        {PaletteEnum::MAGIC_PLUM,          "Magic Plum",         MagicPlum_gp},
        {PaletteEnum::MARINE_MYSTERY,      "Marine Mystery",     MarineMystery_gp},
        {PaletteEnum::MIDNIGHT_BLUE,       "Midnight Blue",      MidnightBlue_gp},
        {PaletteEnum::MIDNIGHT_CALM,       "Midnight Calm",      MidnightCalm_gp},
        {PaletteEnum::MINT_BREEZE,         "Mint Breeze",        MintyBreeze_gp},
        {PaletteEnum::MISTY_MOOR,          "Misty Moor",         MistyMoor_gp},
        {PaletteEnum::MONOCHROME_CHIC,     "Monochrome Chic",    MonochromeChic_gp},
        {PaletteEnum::MOSSY_WHISPER,       "Mossy Whisper",      MossyWhisper_gp},
        {PaletteEnum::MYSTERICAL_SPECTRUM, "Mystical Spectrum",  MysticalSpectrum_gp},
        {PaletteEnum::MYSTIC_MIDNIGHT,     "Mystic Midnight",    MysticMidnight_gp},
        {PaletteEnum::NATURAL_BREEZE,      "Natural Breeze",     NaturalBreeze_gp},
        {PaletteEnum::NEON_DREAMS,         "Neon Dreams",        NeonDreams_gp},
        {PaletteEnum::NEON_SPECTRUM,       "Neon Spectrum",      NeonSpectrum_gp},
        {PaletteEnum::NIGHT_NOCTURNE,      "Night Nocturne",     NightNocturne_gp},
        {PaletteEnum::OCEANIC_BURST,       "Oceanic Burst",      OceanicBurst_gp},
        {PaletteEnum::OCEAN_ADVENTURE,     "Ocean Adventure",    OceanAdventure_gp},
        {PaletteEnum::OCEAN_LOOK,          "Ocean Look",         OceanLook_gp},
        {PaletteEnum::PASTEL_DREAMS,       "Pastel Dreams",      PastelDreams_gp},
        {PaletteEnum::PEACEFUL_OLIVE,      "Peaceful Olive",     PeacefulOlive_gp},
        {PaletteEnum::PURPLE_SKY,          "Purple Sky",         PurpleSky_gp},
        {PaletteEnum::QUIET_WOODS,         "Quiet Woods",        QuietWoods_gp},
        {PaletteEnum::RASPBERRY_SORBET,    "Raspberry Sorbet",   RaspberrySorbet_gp},
        {PaletteEnum::RED_DANCE,           "Red Dance",          RedDance_gp},
        {PaletteEnum::RED_INFERNO,         "Red Inferno",        RedInferno_gp},
        {PaletteEnum::RED_SUNSET,          "Red Sunset",         RedSunset_gp},
        {PaletteEnum::RED_TWILIGHT,        "Red Twilight",       RedTwilight_gp},
        {PaletteEnum::ROSY_DAWN,           "Rosy Dawn",          RosyDawn_gp},
        {PaletteEnum::ROYAL_ELEGANCE,      "Royal Elegance",     RoyalElegance_gp},
        {PaletteEnum::RUSTIC_TERRAIN,      "Rustic Terrain",     RusticTerrain_gp},
        {PaletteEnum::SCARLET_ESSENCE,     "Scarlet Essence",    ScarletEssence_gp},
        {PaletteEnum::SEA_DUSK,            "Sea Dusk",           SeaDusk_gp},
        {PaletteEnum::SHADOW_BLAZE,        "Shadow Blaze",       ShadowBlaze_gp},
        {PaletteEnum::SOFT_PETALS,         "Soft Petals",        SoftPetals_gp},
        {PaletteEnum::SUNNY_MOSAIC,        "Sunny Mosaic",       SunnyMosaic_gp},
        {PaletteEnum::TWILIGHT_SIMPHONY,   "Twilight Simphony",  TwilightSymphony_gp},
        {PaletteEnum::URBAN_DUSK,          "Urban Dusk",         UrbanDusk_gp},
        {PaletteEnum::VIBRANT_TROPICS,     "Vibrant Tropics",    VibrantTropics_gp},
        {PaletteEnum::VINTAGE_ROUGE,       "Vintage Rouge",      VintageRouge_gp},
        {PaletteEnum::VIOLET_TWILIGHT,     "Violet Twilight",    VioletTwilight_gp},
        {PaletteEnum::WARM_ORANGE,         "Warm Orange",        WarmAmber_gp},

        {PaletteEnum::CUSTOM,              "Custom",             DefaultCustomPalette_gp},
};

PaletteConfig Palettes = {
        .count = (uint8_t) palette_init.size(),
        .entries = palette_init
};