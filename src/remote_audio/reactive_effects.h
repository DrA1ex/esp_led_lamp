#pragma once
#include "misc/led.h"
#include "audio/transfer.h"
#include "reactive_math.h"
#include "controller.h"

namespace RemoteReactive {
inline void apply(Led &led, const RemoteAudioController &controller, AudioEffectEnum mode,
                  float mix = 1.f) {
    if (mode != AudioEffectEnum::BASS_MID_HIGH && mode != AudioEffectEnum::BEAT_REACTIVE)
        return;
    const auto &f = controller.timeline().output();
    const auto *pulses = controller.pulses();
    const uint16_t width = led.width();
    const uint16_t height = led.height();
    if (!width || !height) return;
    const OriginMap origin(width, height, controller.overrides().tuning.origin);
    const float beat = predictedBeat(f.beat_phase, f.beat_confidence, pulses[0], f.bpm_x10);
    for (uint16_t x = 0; x < width; ++x) {
        const int xp = origin.x(x);
        if (xp < 0) {
            const auto dark = static_cast<uint8_t>(clamp01(1.f - mix) * 255.f);
            for (uint16_t y = 0; y < height; ++y) led.get_pixel(x, y).nscale8(dark);
            continue;
        }
        // Lab features[x * 16 / W] are aggregated from the same logarithmic
        // FFT; remote wire format has 32 bands, mapped over the full width.
        const uint8_t index = uint32_t(xp) * RemoteProtocol::BAND_COUNT / width;
        const uint16_t band = f.bands[index < 32 ? index : 31];
        for (uint16_t y = 0; y < height; ++y) {
            const uint8_t scale = mode == AudioEffectEnum::BEAT_REACTIVE
                    ? beatReactiveAt(band, beat, origin.beatRadius(xp, y), mix)
                    : bassMidHigh(band, f.bass, f.mid, f.high, pulses[0], pulses[1], pulses[2],
                                  halfForRow(y, height), mix);
            led.get_pixel(x, y).nscale8(scale);
        }
    }
}

// Whether render() is needed instead of the stock AudioEffectManager path,
// which already handles the signal window.
inline bool customRender(const RemoteAudioController &controller, AudioEffectEnum mode) {
    const auto &tuning = controller.overrides().tuning;
    switch (mode) {
        case AudioEffectEnum::WAVE:
        case AudioEffectEnum::WAVE_CENTERED:
        case AudioEffectEnum::SPECTRUM:
        case AudioEffectEnum::SPECTRUM_CENTERED:
            return tuning.spectrum_shape || tuning.spectrum_onset || !tuning.origin.is_default();
        case AudioEffectEnum::LINE:
        case AudioEffectEnum::LINE_CENTERED:
            return !tuning.origin.is_default();
        default:
            return false;
    }
}

// Wave / Spectrum / Line and their Center variants with the remote tuning:
// signal window, origin (shift / reverse / ring / radial centre), and the
// lab's "Spectrum expression" / "Spectrum onset". Value scaling is
// AudioEffectManager's; with default tuning the output equals the stock one.
inline void render(Led &led, const SignalProvider &provider, const RemoteAudioController &controller,
                   AudioEffectEnum mode, uint8_t v_min, uint8_t v_max) {
    const auto &tuning = controller.overrides().tuning;
    const int width = led.width();
    const int height = led.height();
    if (!width || !height) return;
    const OriginMap origin(width, height, tuning.origin);
    const uint16_t max_value = provider.max_value();

    if (mode == AudioEffectEnum::LINE || mode == AudioEffectEnum::LINE_CENTERED) {
        const bool centered = mode == AudioEffectEnum::LINE_CENTERED;
        const int reach = audioTransfer(provider.get(0, 255), max_value, centered ? width / 2 : width,
                                        v_min, v_max, tuning.window);
        for (int x = 0; x < width; ++x) {
            const int xp = origin.x(x);
            if (!centered) {
                if (xp < 0 || xp >= reach) led.fill_column(x, CRGB::Black);
                continue;
            }
            for (int y = 0; y < height; ++y)
                if (xp < 0 || !origin.lineCenteredLit(xp, y, reach)) led.get_pixel(x, y) = CRGB::Black;
        }
        return;
    }

    const bool centered = mode == AudioEffectEnum::WAVE_CENTERED || mode == AudioEffectEnum::SPECTRUM_CENTERED;
    const float shape = tuning.spectrum_shape / 100.f;
    const float onset = tuning.spectrum_onset / 100.f;
    const auto *pulses = controller.pulses();
    const uint16_t interval = provider.update_interval();
    const uint8_t frac = interval ? min<uint32_t>(255, uint32_t(provider.delta()) * 255 / interval) : 255;
    // A radial centre differs per row; otherwise a whole column shares a factor.
    const bool per_pixel = centered && origin.radial();
    for (int x = 0; x < width; ++x) {
        const int xp = origin.x(x);
        if (xp < 0) {
            led.fill_column(x, CRGB::Black);
            continue;
        }
        for (int y = 0; y < (per_pixel ? height : 1); ++y) {
            const uint16_t index = centered ? origin.centeredIndex(xp, y) : xp;
            const uint8_t value = audioTransfer(provider.get(index, frac), max_value, 255,
                                                v_min, v_max, tuning.window);
            const float accent = onset > 0.f
                    ? onset * .22f * pulses[onsetGroup(index, width)] / RemoteProtocol::MAX_LEVEL : 0.f;
            const uint8_t factor = shapedLevel(value, shape, accent);
            if (per_pixel) led.get_pixel(x, y).nscale8(factor);
            else for (int row = 0; row < height; ++row) led.get_pixel(x, row).nscale8(factor);
        }
    }
}
} // namespace RemoteReactive
