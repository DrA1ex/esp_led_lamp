#pragma once

#include "config.h"
#include "debug.h"
#include "led.h"
#include "type.h"

class BrightnessEffectManager : public FxManagerBase<BrightnessEffectConfig, BrightnessEffectState> {
public:
    BrightnessEffectManager();

    void call(Led &led, const Config &config);

    static void eco(Led &led, byte level);

protected:
    void _reset_state() override;

private:
    static void fixed(Led &led, BrightnessEffectState &state);
    static void pulse(Led &led, BrightnessEffectState &state);
    static void wave(Led &led, BrightnessEffectState &state);
    static void double_wave(Led &led, BrightnessEffectState &state);
};