#pragma once

#include "config.h"
#include "debug.h"
#include "type.h"

class BrightnessEffectManager : public FxManagerBase<BrightnessEffectConfig, BrightnessEffectState> {
public:
    BrightnessEffectManager();

    void call(Led &led, const Config &config);

    static void eco(Led &led, byte level);

protected:
    static void fixed(Led &led, BrightnessEffectState &state);
    static void pulse(Led &led, BrightnessEffectState &state);
    static void wave(Led &led, BrightnessEffectState &state);
    static void double_wave(Led &led, BrightnessEffectState &state);
    static void oscillator(Led &led, BrightnessEffectState &state);

private:
    static uint8_t _pulse_phase(BrightnessEffectState &state, uint8_t scale, uint16_t offset = 0);
};