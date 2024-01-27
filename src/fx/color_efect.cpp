#include "color_effect.h"

#include "misc/led.h"
#include "utils/palette.h"

ColorEffectManager::ColorEffectManager() {
    static constexpr std::initializer_list<ColorEffectEntry> fx_init = {
            {ColorEffectEnum::PERLIN,       "Perlin Noise", perlin},
            {ColorEffectEnum::GRADIENT,     "Gradient",     gradient},
            {ColorEffectEnum::FIRE,         "Fire",         fire},
            {ColorEffectEnum::AURORA,       "Aurora",       aurora},
            {ColorEffectEnum::PLASMA,       "Plasma",       plasma},
            {ColorEffectEnum::PARTICLES,    "Particles",    particles},
            {ColorEffectEnum::CHANGE_COLOR, "Color Change", color_change},
            {ColorEffectEnum::SOLID,        "Solid Color",  solid},
    };

    static_assert(check_entry_order(fx_init), "Order isn't valid: item index must be the same as the code");

    _config.entries = fx_init;
    _config.count = _config.entries.size();
}

void ColorEffectManager::call(Led &led, const PaletteT *palette, const PresetConfig &config) {
    _before_call();

    _state.params.speed = config.speed;
    _state.params.scale = config.scale;
    _state.params.palette = palette;

    _config.entries[(int) _fx].value(led, _state);
    _after_call();
}

void ColorEffectManager::perlin(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    state.current_time_factor = state.prev_time_factor + (double) state.delta() * speed / 255;
    apply_period(state.current_time_factor, 1LL << 24);

    double scale_factor = scale / 2.0;

    if (height > 1) {
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                auto noise_value = inoise_hd(i * scale_factor, j * scale_factor, state.current_time_factor);
                led.setPixel(i, j, color_from_palette(*palette, noise_value));
            }
        }
    } else {
        for (int i = 0; i < width; ++i) {
            auto noise_value = inoise_hd(i * scale_factor, state.current_time_factor);
            led.setPixel(i, 0, color_from_palette(*palette, noise_value));
        }
    }
}

void ColorEffectManager::solid(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    auto color = CHSV(speed, scale, 255);

    led.fillSolid(color);
}

void ColorEffectManager::color_change(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    state.current_time_factor = state.prev_time_factor + (double) state.delta() * speed / 10 / 255;
    uint16_t value = apply_period(state.current_time_factor, 1LL << 24);

    auto color = color_from_palette(*palette, value * 16);
    led.fillSolid(color);
}

void ColorEffectManager::gradient(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    state.current_time_factor = state.prev_time_factor + (double) state.delta() * (speed - 128) / 4 / 128;
    apply_period(state.current_time_factor, 1LL << 24);

    auto scale_factor = ((float) scale * 1.9f + 25) / (float) led.width();

    for (int i = 0; i < led.width(); i++) {
        auto index = ((double) i * scale_factor + state.current_time_factor) * 16;
        auto color = color_from_palette(*palette, (uint16_t) index);
        led.fillColumn(i, color);
    }
}

struct Particle {
    int x = 0;
    int y = 0;
    byte brightness = 0;
    CRGB color = CRGB::Black;
};

static Particle particles_store[MAX_PARTICLES_COUNT];

void ColorEffectManager::particles(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    const uint16_t particle_count = 1 + scale * (MAX_PARTICLES_COUNT - 1) / 255;
    const int fade_speed = 1 + speed / 8;

    for (uint16_t k = 0; k < particle_count; ++k) {
        auto &particle = particles_store[k];

        if (particle.brightness == 0 && random8() < 32) {
            particle.x = random8() % width;
            particle.y = random8() % height;

            particle.color = ColorFromPalette(*palette, random8());
            particle.brightness = 255;
        }

        const auto color = particle.color.scale8(particle.brightness);
        auto &pixel = led.getPixel(particle.x, particle.y);
        nblend(pixel, color, 128);

        particle.brightness = ((uint16_t) particle.brightness * (255 - fade_speed)) >> 8;
    }
}

void ColorEffectManager::fire(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    state.current_time_factor = state.prev_time_factor + (double) state.delta() * speed / 255;
    apply_period(state.current_time_factor, (1LL << 24) - 1);

    auto time_factor = state.current_time_factor;
    auto scale_factor = scale / 2.0;

    for (int j = 0; j < height; j++) {
        const auto height_limit = (j * 4096 / height);
        const auto noise_y = (j * scale_factor) - time_factor;

        for (int i = 0; i < width; i++) {
            const auto noise_x = i * scale_factor;
            const auto noise_value = inoise_hd(noise_x, noise_y);
            auto color_index = noise_value - height_limit;

            uint8_t brightness = 255;
            if (color_index < 0) {
                brightness = min(255, max(0, 255 + color_index / 2));
                color_index = 0;
            }

            auto color = color_from_palette(*palette, color_index, brightness);
            led.setPixel(i, j, color);
        }
    }
}

void ColorEffectManager::aurora(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * (float) speed / 255;
    const auto time_factor = apply_period(state.current_time_factor, 1 << 16);

    auto scale_factor = scale / 2;
    auto height_factor = height * 185 / 100;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            const auto value = inoise8(
                    i * scale_factor,
                    j * height,
                    time_factor);

            const auto color = ColorFromPalette(*palette, qsub8(value, height_factor * abs(height / 2 - j)));
            led.setPixel(i, j, color);
        }
    }
}

float sin8f(float i) {
    while (i > 65535) {
        i -= 65535;
    }

    return (sin16((uint16_t) (i / 255 * 65535)) / 32767.0f + 1) * 255;
}

void ColorEffectManager::plasma(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * (float) speed / 128 / 255;
    apply_period(state.current_time_factor, 1 << 16);

    const auto time_factor = (float) state.current_time_factor;
    const float scale_factor = ((float) scale + 1.f) / 256;

    auto x_drift = time_factor * 5;
    auto y_drift = x_drift;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            const auto index = sin8f(x_drift + i * 8 + sin8f(i * 2 + time_factor * 6)) / 2 +
                               sin8f(y_drift + j * 8 + sin8f(j * 2 + time_factor * 7) / 2);

            const auto color = ColorFromPalette(*palette, (int) (index * scale_factor) % 256);
            led.setPixel(i, j, color);
        }
    }
}
