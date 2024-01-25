#include "color_effect.h"

#include "misc/led.h"

ColorEffectManager::ColorEffectManager() {
    static constexpr std::initializer_list<ColorEffectEntry> fx_init = {
            {ColorEffectEnum::PERLIN,       "Perlin Noise", perlin},
            {ColorEffectEnum::GRADIENT,     "Gradient",     gradient},
            {ColorEffectEnum::PACIFIC,      "Fire",         fire},
            {ColorEffectEnum::PARTICLES,    "Particles",    particles},
            {ColorEffectEnum::CHANGE_COLOR, "Color Change", changeColor},
            {ColorEffectEnum::SOLID,        "Solid Color",  solid},
    };

    static_assert(check_entry_order(fx_init), "Order isn't valid: item index must be the same as the code");

    _config.entries = fx_init;
    _config.count = _config.entries.size();
}

void ColorEffectManager::call(Led &led, const CRGBPalette16 *palette, const PresetConfig &config) {
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

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * speed / 4 / 255;
    const auto time_factor = apply_period(state.current_time_factor, (1 << 16) - 1);

    float scale_factor = scale / 3.f;

    if (height > 1) {
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                led.setPixel(i, j, ColorFromPalette(
                        *palette,
                        inoise8(
                                (float) i * scale_factor - (float) width * scale_factor / 4.f,
                                (float) j * scale_factor - (float) height * scale_factor / 4.f,
                                time_factor))
                );
            }
        }
    } else {
        for (int i = 0; i < width; ++i) {
            led.setPixel(i, 0, ColorFromPalette(
                    *palette,
                    inoise8(
                            (float) i * scale_factor - (float) width * scale_factor / 4.f,
                            time_factor)
            ));
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

void ColorEffectManager::changeColor(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * speed / 10.f / 255.f;
    const byte value = apply_period(state.current_time_factor, 256);

    auto color = ColorFromPalette(*palette, value);
    led.fillSolid(color);
}

void ColorEffectManager::gradient(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * (speed - 128) / 8.f / 128.f;
    const auto time_factor = apply_period(state.current_time_factor, 256);

    auto scale_factor = ((float) scale * 1.9f + 25) / (float) led.width();

    for (int i = 0; i < led.width(); i++) {
        auto index = (long) ((float) i * scale_factor) + time_factor;
        auto color = ColorFromPalette(*palette, index % 256);
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
            particle.x = random16() % width;
            particle.y = random16() % height;

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

    state.current_time_factor = state.prev_time_factor + (float) state.delta() * (float) speed / 2 / 255;
    const auto time_factor = apply_period(state.current_time_factor, (1 << 16) - 1);

    auto scale_factor = scale / 2;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            const auto brightness = max(0, (int16_t) inoise8(
                    (uint16_t) (i * scale_factor),
                    (uint16_t) (j * scale_factor) - time_factor,
                    time_factor) - (j * 255 / height));

            const uint8_t color_index = brightness % 256;

            auto color = ColorFromPalette(*palette, color_index, brightness > 0 ? 255 - brightness / 5 : 0);
            nblend(led.getPixel(i, j), color, 176);
        }
    }
}
