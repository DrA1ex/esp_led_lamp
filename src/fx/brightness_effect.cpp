#include "brightness_effect.h"

#include "misc/led.h"

BrightnessEffectManager::BrightnessEffectManager() {
    static constexpr std::initializer_list<BrightnessEffectEntry> fx_init = {
            {BrightnessEffectEnum::FIXED,       "Fixed",       fixed},
            {BrightnessEffectEnum::PULSE,       "Pulse",       pulse},
            {BrightnessEffectEnum::WAVE,        "Wave",        wave},
            {BrightnessEffectEnum::DOUBLE_WAVE, "Double Wave", double_wave},
            {BrightnessEffectEnum::OSCILLATOR,  "Oscillator",  oscillator},
    };

    static_assert(check_entry_order(fx_init), "Order isn't valid: item index must be the same as the code");

    _config.entries = fx_init;
    _config.count = _config.entries.size();
}

void BrightnessEffectManager::call(Led &led, const PresetConfig &config) {
    _before_call();

    _state.params.level = config.light;

    const auto fx_function = _config.entries[(int) _fx].value;
    fx_function(led, _state);

    _after_call();
}

void BrightnessEffectManager::fixed(Led &led, BrightnessEffectState &state) {}

void BrightnessEffectManager::pulse(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    const auto phase = _pulse_phase(state, level);
    auto brightness = ease8InOutQuad(phase);

    led.fade_to_black(255 - brightness);
}

uint8_t BrightnessEffectManager::_pulse_phase(BrightnessEffectState &state, uint8_t scale, uint16_t offset) {
    state.current_time_factor = state.prev_time_factor + (float) state.delta() * scale / 2.f / 255.f;
    const auto value = (offset + apply_period(state.current_time_factor, 1024)) % 1024;

    byte phase;
    if (value < 256) {              // Fade out
        phase = 255 - value;
    } else if (value < 512) {       // Inactive
        phase = 0;
    } else if (value < 768) {       // Fade in
        phase = value - 512;
    } else {                        // Active
        phase = 255;
    }

    return phase;
}

void BrightnessEffectManager::wave(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * (level - 128) / 2.f / 255.f;
    const auto value = apply_period(state.current_time_factor, 256);

    for (int i = 0; i < led.width(); ++i) {
        const auto brightness = cubicwave8((value + i * 6) % 256);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.get_pixel(i, j);
            color.fadeToBlackBy(brightness);
        }
    }
}

void BrightnessEffectManager::double_wave(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * (level - 128) / 255.f;
    const auto value = apply_period(state.current_time_factor, 512);

    const auto &width = led.width();
    const int half_width = ceil(width / 2.0f);

    for (int i = 0; i < half_width; ++i) {
        const auto phase = (value + i * 32) % 512;
        const byte brightness = 255 - (phase < 256 ? cubicwave8(phase) : 0);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.get_pixel(i, j);
            color.fadeToBlackBy(brightness);

            const auto i2 = width - i - 1;
            if (i2 >= half_width) {
                auto &color2 = led.get_pixel(i2, j);
                color2.fadeToBlackBy(brightness);
            }
        }
    }
}

void BrightnessEffectManager::eco(Led &led, byte level) {
    const auto total_count = led.count();
    const auto active_count = (total_count * level) / 255;

    if (total_count == active_count) return;

    const auto invert = active_count > total_count / 2;
    const float period = !invert ? (float) total_count / (float) (active_count + 1)
                                 : (float) total_count / (float) (total_count - active_count + 1);

    float next_index = period;
    for (int j = 0; j < led.height(); ++j) {
        for (int i = 0; i < led.width(); ++i) {
            const auto index = i + j * led.width();
            if (index < (int) next_index) {
                if (!invert) led.set_pixel(i, j, CRGB::Black);
            } else {
                if (invert) led.set_pixel(i, j, CRGB::Black);
                next_index += period;
            }
        }
    }
}

void BrightnessEffectManager::oscillator(Led &led, BrightnessEffectState &state) {
    const auto &[level] = state.params;

    const auto phase1 = _pulse_phase(state, 32);
    const auto phase2 = _pulse_phase(state, 32, 512);

    const auto brightness1 = ease8InOutQuad(phase1);
    const auto brightness2 = ease8InOutQuad(phase2);

    const auto period = max(1, led.count() * max<int>(1, level) / 255);

    int k = 0;
    for (int i = 0; i < led.width(); ++i) {
        for (int j = 0; j < led.height(); ++j) {
            const auto group = (k++ / period) % 2;

            auto &color = led.get_pixel(i, j);
            color.fadeToBlackBy(group ? brightness1 : brightness2);
        }
    }
}

size_t BrightnessEffectManager::debug(char *dst, size_t length) {
    size_t offset = snprintf(dst, length, "BrightnessEffectManager:\n");
    offset += FxManagerBase::debug(dst + offset, length - offset);

    return offset;
}
