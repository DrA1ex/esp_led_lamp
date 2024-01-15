#include "brightness_effect.h"

BrightnessEffectManager::BrightnessEffectManager() {
    _config.entries = {
            {BrightnessEffectEnum::FIXED,       "Fixed",       fixed},
            {BrightnessEffectEnum::PULSE,       "Pulse",       pulse},
            {BrightnessEffectEnum::WAVE,        "Wave",        wave},
            {BrightnessEffectEnum::DOUBLE_WAVE, "Double Wave", double_wave},
            {BrightnessEffectEnum::ECO,         "ECO",         eco}
    };

    _config.count = _config.entries.size();
}

void BrightnessEffectManager::call(Led &led, const Config &config) {
    _before_call();

    _state.params.level = config.light;

    const auto fx_function = _config.entries[(int) _fx].value;
    fx_function(led, _state);

    _save_next_value(_state.prev_level_value, _state.current_level_value);
    _after_call();
}

void BrightnessEffectManager::_reset_state() {
    _state.current_level_value = 0;
    _state.prev_level_value = 0;
}

void BrightnessEffectManager::fixed(Led &led, BrightnessEffectState &state) {}

void BrightnessEffectManager::pulse(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    state.current_level_value = state.prev_level_value + (float) state.delta() * level / 2.f / 255.f;
    const auto value = _apply_period(state.current_level_value, 768);

    byte phase;
    if (value < 256) { // Fade out
        phase = 255 - value;
    } else if (value < 512) { // Fade in
        phase = value - 256;
    } else { // Active out
        phase = 255;
    }

    auto brightness = ease8InOutQuad(phase);
    led.fadeToBlack(255 - brightness);
}

void BrightnessEffectManager::wave(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    state.current_level_value = state.prev_level_value + (float) state.delta() * level / 2.f / 255.f;
    const auto value = _apply_period(state.current_level_value, 256);

    for (int i = 0; i < led.width(); ++i) {
        const auto brightness = cubicwave8((value + i * 6) % 256);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.getPixel(i, j);
            color.fadeToBlackBy(brightness);
        }
    }
}

void BrightnessEffectManager::double_wave(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    state.current_level_value = state.prev_level_value + (float) state.delta() * level / 2.f / 255.f;
    const auto value = _apply_period(state.current_level_value, 512);

    const auto &width = led.width();
    const int half_width = ceil(width / 2.0f);

    for (int i = 0; i < half_width; ++i) {
        const auto phase = (value + i * 32) % 512;
        const byte brightness = 255 - (phase < 256 ? cubicwave8(phase) : 0);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.getPixel(i, j);
            color.fadeToBlackBy(brightness);

            const auto i2 = width - i - 1;
            if (i2 >= half_width) {
                auto &color2 = led.getPixel(i2, j);
                color2.fadeToBlackBy(brightness);
            }
        }
    }
}

void BrightnessEffectManager::eco(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    const auto total_count = led.width() * led.height();
    const auto active_count = (total_count * level) / 255;

    if (total_count == active_count) return;

    const auto invert = active_count > total_count / 2;
    const float period = !invert ? (float) total_count / (float) (active_count + 1)
                                 : (float) total_count / (float) (total_count - active_count + 1);

    float next_index = period;
    for (int i = 0; i < led.width(); ++i) {
        for (int j = 0; j < led.height(); ++j) {
            if (i < (int) next_index) {
                if (!invert) led.setPixel(i, j, CRGB::Black);
            } else {
                if (invert) led.setPixel(i, j, CRGB::Black);
                next_index += period;
            }
        }
    }
}
