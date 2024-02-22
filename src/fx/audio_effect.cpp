#include "audio_effect.h"

AudioEffectManager::AudioEffectManager() {
    static constexpr std::initializer_list<AudioEffectEntry> fx_init = {
            {AudioEffectEnum::WAVE,              "Wave",            signal},
            {AudioEffectEnum::WAVE_CENTERED,     "Wave Center",     signal_centered},
            {AudioEffectEnum::SPECTRUM,          "Spectrum",        signal},
            {AudioEffectEnum::SPECTRUM_CENTERED, "Spectrum Center", signal_centered},
            {AudioEffectEnum::BRIGHTNESS,        "Brightness",      brightness},
            {AudioEffectEnum::LINE,              "Line",            line},
            {AudioEffectEnum::LINE_CENTERED,     "Line Center",     line_centered},
            {AudioEffectEnum::SPEED_CONTROL,     "Speed Control",   _none},
            {AudioEffectEnum::SCALE_CONTROL,     "Scale Control",   _none},
            {AudioEffectEnum::LIGHT_CONTROL,     "Light Control",   _none},
    };

    static_assert(check_entry_order(fx_init), "Order isn't valid: item index must be the same as the code");

    _config.entries = fx_init;
    _config.count = _config.entries.size();
}

void AudioEffectManager::call(Led &led, const SignalProvider *provider, uint8_t min_value, uint8_t max_value) {
    _before_call();

    _state.params.provider = provider;
    _state.params.min = min_value;
    _state.params.max = max_value;

    _config.entries[(int) _fx].value(led, _state);

    _after_call();
}

void AudioEffectManager::signal(Led &led, AudioEffectState &state) {
    const auto [
            provider, v_min, v_max
    ] = state.params;

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
    const auto [
            provider, v_min, v_max
    ] = state.params;

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

void AudioEffectManager::line(Led &led, AudioEffectState &state) {
    const auto [
            provider, v_min, v_max
    ] = state.params;

    const auto value = provider->get(0, 255);
    const auto width = value * led.width() / provider->max_value();

    for (int i = width; i < led.width(); ++i) {
        led.fill_column(i, CRGB::Black);
    }
}

void AudioEffectManager::line_centered(Led &led, AudioEffectState &state) {
    const auto [
            provider, v_min, v_max
    ] = state.params;

    const auto value = provider->get(0, 255);
    const auto width = value * led.width() / 2 / provider->max_value();

    const auto inactive_width = led.width() / 2 - width;
    for (int i = 0; i < inactive_width; ++i) {
        led.fill_column(i, CRGB::Black);
        led.fill_column(led.width() - i - 1, CRGB::Black);
    }
}

void AudioEffectManager::brightness(Led &led, AudioEffectState &state) {
    const auto [
            provider, v_min, v_max
    ] = state.params;

    const auto value = provider->get(0, 255);
    const auto brightness = dim8_raw(value * 255 / provider->max_value());

    for (int i = 0; i < led.width(); ++i) {
        for (int j = 0; j < led.height(); ++j) {
            led.get_pixel(i, j).nscale8(brightness);
        }
    }
}

void AudioEffectManager::_none(Led &, AudioEffectState &) {}
