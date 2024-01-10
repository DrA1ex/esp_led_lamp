#include "fx.h"

#include "brightness_effect.h"
#include "color_effect.h"
#include "palette.h"

CRGBPalette16 Palettes[] = {
        CustomPal,
        HeatColors_p,
        Fire_gp,
        WoodFireColors_p,
        NormalFire_p,
        LithiumFireColors_p,
        SodiumFireColors_p,
        CopperFireColors_p,
        AlcoholFireColors_p,
        LavaColors_p,
        PartyColors_p,
        RainbowColors_p,
        RainbowStripeColors_p,
        CloudColors_p,
        OceanColors_p,
        ForestColors_p,
        Sunset_Real_gp,
        Dkbluered_gp,
        Optimus_Prime_gp,
        WarmGrad_gp,
        ColdGrad_gp,
        HotGrad_gp,
        PinkGrad_gp,
        Comfy_gp,
        Cyberpunk_gp,
        Girl_gp,
        Xmas_gp,
        Acid_gp,
        BlueSmoke_gp,
        Gummy_gp,
        Leo_gp,
        Aurora_gp,
};

ColorEffectFn ColorEffects[] = {
        perlin,
        gradient,
        pacific,
        particles,
        changeColor,
        solid,
};

BrightnessEffectFn BrightnessEffects[] = {
        fixed,
        pulse,
        wave,
        double_wave
};