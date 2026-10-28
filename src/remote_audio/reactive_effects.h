#pragma once
#include "misc/led.h"
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
    const float beat = predictedBeat(f.beat_phase, f.beat_confidence, pulses[0]);
    for (uint16_t x = 0; x < width; ++x) {
        // Lab features[x * 16 / W] are aggregated from the same logarithmic
        // FFT; remote wire format has 32 bands, mapped over the full width.
        const uint8_t index = uint32_t(x) * RemoteProtocol::BAND_COUNT / width;
        const uint16_t band = f.bands[index < 32 ? index : 31];
        if (mode == AudioEffectEnum::BEAT_REACTIVE) {
            const uint8_t scale = beatReactive(band, beat, x, width, mix);
            for (uint16_t y = 0; y < height; ++y) led.get_pixel(x, y).nscale8(scale);
        } else {
            const uint8_t upper = bassMidHigh(band, f.bass, f.mid, f.high,
                                               pulses[0], pulses[1], pulses[2], true, mix);
            const uint8_t lower = bassMidHigh(band, f.bass, f.mid, f.high,
                                               pulses[0], pulses[1], pulses[2], false, mix);
            for (uint16_t y = 0; y < height; ++y)
                led.get_pixel(x, y).nscale8(uint32_t(y) * 2 < height ? upper : lower);
        }
    }
}
} // namespace RemoteReactive
