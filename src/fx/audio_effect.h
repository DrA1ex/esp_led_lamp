#pragma once

#include "config.h"

class AudioEffectManager : public FxManagerBase<AudioEffectConfig, AudioEffectState> {
public:
    AudioEffectManager();

    void call(Led &led, const SignalProvider *provider, uint8_t min_value, uint8_t max_value);

protected:
    static void signal(Led &led, AudioEffectState &state);
    static void signal_centered(Led &led, AudioEffectState &state);

    static void line(Led &led, AudioEffectState &state);
    static void line_centered(Led &led, AudioEffectState &state);

    static void brightness(Led &led, AudioEffectState &state);

    static void _none(Led &, AudioEffectState &);
};
