#pragma once

#include "config.h"

class ColorEffectManager : public FxManagerBase<ColorEffectConfig, ColorEffectState> {
public:
    ColorEffectManager();

    void call(Led &led, const PaletteT *palette, const PresetConfig &config, uint8_t gamma);

    size_t debug(char *dst, size_t length) override;

private:
    static void perlin(Led &led, ColorEffectState &state);
    static void gradient(Led &led, ColorEffectState &state);
    static void fire(Led &led, ColorEffectState &state);
    static void aurora(Led &led, ColorEffectState &state);
    static void plasma(Led &led, ColorEffectState &state);
    static void particles(Led &led, ColorEffectState &state);
    static void ripple(Led &led, ColorEffectState &state);
    static void velum(Led &led, ColorEffectState &state);
    static void color_change(Led &led, ColorEffectState &state);
    static void solid(Led &led, ColorEffectState &state);
};