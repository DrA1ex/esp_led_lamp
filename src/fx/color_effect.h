#pragma once

#include "config.h"

class ColorEffectManager : public FxManagerBase<ColorEffectConfig, ColorEffectState> {
public:
    ColorEffectManager();

    void call(Led &led, const CRGBPalette16 *palette, const PresetConfig &config);

private:
    static void perlin(Led &led, ColorEffectState &state);
    static void gradient(Led &led, ColorEffectState &state);
    static void fire(Led &led, ColorEffectState &state);
    static void aurora(Led &led, ColorEffectState &state);
    static void plasma(Led &led, ColorEffectState &state);
    static void particles(Led &led, ColorEffectState &state);
    static void changeColor(Led &led, ColorEffectState &state);
    static void solid(Led &led, ColorEffectState &state);
};