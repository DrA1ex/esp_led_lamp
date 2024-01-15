#pragma once

#include "config.h"
#include "led.h"

class ColorEffectManager {
    ColorEffectConfig _config;
    ColorEffectEnum _fx = (ColorEffectEnum) 0;
    ColorEffectState _state;

public:
    ColorEffectManager();

    void call(Led &led, const CRGBPalette16 *palette, const Config &config);
    void select(ColorEffectEnum fx);

    [[nodiscard]] inline const ColorEffectConfig &config() const { return _config; }

private:
    static void perlin(Led &led, ColorEffectState &state);
    static void gradient(Led &led, ColorEffectState &state);
    static void pacific(Led &led, ColorEffectState &state);
    static void particles(Led &led, ColorEffectState &state);
    static void changeColor(Led &led, ColorEffectState &state);
    static void solid(Led &led, ColorEffectState &state);
};