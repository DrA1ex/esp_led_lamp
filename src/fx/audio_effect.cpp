#include "audio_effect.h"

AudioEffectManager::AudioEffectManager() {
    static constexpr std::initializer_list<AudioEffectEntry> fx_init = {
            {AudioEffectEnum::WAVE,              "Wave",              signal},
            {AudioEffectEnum::WAVE_CENTERED,     "Wave Centered",     signal_centered},
            {AudioEffectEnum::SPECTRUM,          "Spectrum",          signal},
            {AudioEffectEnum::SPECTRUM_CENTERED, "Spectrum Centered", signal_centered},
            {AudioEffectEnum::SPEED_CONTROL,     "Speed Control",     _none},
            {AudioEffectEnum::SCALE_CONTROL,     "Scale Control",     _none},
            {AudioEffectEnum::LIGHT_CONTROL,     "Light Control",     _none},
    };

    static_assert(check_entry_order(fx_init), "Order isn't valid: item index must be the same as the code");

    _config.entries = fx_init;
    _config.count = _config.entries.size();
}

void AudioEffectManager::call(Led &led, const SignalProvider *provider) {
    _before_call();

    _state.params.provider = provider;

    _config.entries[(int) _fx].value(led, _state);

    _after_call();
}

void AudioEffectManager::signal(Led &led, AudioEffectState &state) {
    const auto [provider] = state.params;

    const auto frac = min<uint8_t>(255, provider->delta() * 255 / provider->update_interval());

    for (int i = 0; i < led.width(); ++i) {
        const auto v = provider->get(i, frac);
        auto brightness = dim8_raw(v * 255 / provider->max_value());

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.get_pixel(i, j);
            color.nscale8(brightness);
        }
    }

}

void AudioEffectManager::signal_centered(Led &led, AudioEffectState &state) {
    const auto [provider] = state.params;
    const auto frac = min<uint8_t>(255, provider->delta() * 255 / provider->update_interval());

    const auto center = led.width() / 2;

    for (int i = 0; i < led.width(); ++i) {
        const auto v = i <= center ? provider->get(center - i, frac) : provider->get(i - center, frac);
        auto brightness = dim8_raw(v * 255 / provider->max_value());

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.get_pixel(i, j);
            color.nscale8(brightness);
        }
    }
}

void AudioEffectManager::_none(Led &, AudioEffectState &) {}
