#pragma once

#include "config.h"

class ColorEffectManager : public FxManagerBase<ColorEffectConfig, ColorEffectState> {
public:
    ColorEffectManager();

    void call(Led &led, const CRGBPalette16 *palette, const Config &config);

private:
    static void perlin(Led &led, ColorEffectState &state);
    static void gradient(Led &led, ColorEffectState &state);
    static void pacific(Led &led, ColorEffectState &state);
    static void particles(Led &led, ColorEffectState &state);
    static void changeColor(Led &led, ColorEffectState &state);
    static void solid(Led &led, ColorEffectState &state);
};