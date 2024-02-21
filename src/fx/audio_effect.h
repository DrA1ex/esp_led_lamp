#pragma once

#include "config.h"

class AudioEffectManager : public FxManagerBase<AudioEffectConfig, AudioEffectState> {
public:
    AudioEffectManager();

    void call(Led &led, const SignalProvider *provider);

protected:
    static void signal(Led &led, AudioEffectState &state);
    static void signal_centered(Led &led, AudioEffectState &state);
    static void _none(Led &, AudioEffectState &);
};
