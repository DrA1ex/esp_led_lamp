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

int AudioEffectManager::calc(Led &led, const SignalProvider *provider, uint8_t min_value, uint8_t max_value) {
    call(led, provider, min_value, max_value);

    return _calc_signal_value(_state, 255);
}

void AudioEffectManager::signal(Led &led, AudioEffectState &state) {
    const auto [
            provider, v_min, v_max
    ] = state.params;

    const auto frac = min<uint8_t>(255, provider->delta() * 255 / provider->update_interval());

    for (int i = 0; i < led.width(); ++i) {
        const auto brightness = dim8_raw(_calc_signal_value(state, 255, i, frac));

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
        const auto index = i <= center ? center - i : i - center;
        const auto brightness = dim8_raw(_calc_signal_value(state, 255, index, frac));

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.get_pixel(i, j);
            color.nscale8(brightness);
        }
    }
}

void AudioEffectManager::line(Led &led, AudioEffectState &state) {
    const auto width = _calc_signal_value(state, led.width());

    for (int i = width; i < led.width(); ++i) {
        led.fill_column(i, CRGB::Black);
    }
}

void AudioEffectManager::line_centered(Led &led, AudioEffectState &state) {
    const auto width = _calc_signal_value(state, led.width() / 2);

    const auto inactive_width = led.width() / 2 - width;
    for (int i = 0; i < inactive_width; ++i) {
        led.fill_column(i, CRGB::Black);
        led.fill_column(led.width() - i - 1, CRGB::Black);
    }
}

void AudioEffectManager::brightness(Led &led, AudioEffectState &state) {
    const auto brightness = dim8_raw(_calc_signal_value(state, 255));

    for (int i = 0; i < led.width(); ++i) {
        for (int j = 0; j < led.height(); ++j) {
            led.get_pixel(i, j).nscale8(brightness);
        }
    }
}

void AudioEffectManager::_none(Led &, AudioEffectState &) {}

int AudioEffectManager::_calc_signal_value(const AudioEffectState &state, int count, uint16_t index, uint8_t frac) {
    auto [
            provider, v_min, v_max
    ] = state.params;

    if (v_min > v_max) v_min = v_max;

    const auto count_min = v_min * count / 255;
    const auto count_max = v_max * count / 255;
    const auto actual = count_max - count_min;

    const auto value = provider->get(index, frac);
    return count_min + value * actual / provider->max_value();
}
