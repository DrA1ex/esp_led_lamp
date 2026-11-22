#pragma once

#include <math.h>
#include <stdint.h>

// Signal window applied before the AudioConfig min/max mapping. The input
// window [in_min, in_max] (0..255 of the provider range) is stretched onto
// [out_min, out_max]; below in_min yields out_min (or 0 with gate), above
// in_max yields out_max. In between, the position t in the window goes
// through the curve t^(curve / 100): > 100 keeps low and mid signals dim and
// spreads the top (LEDs look logarithmic), < 100 lifts quiet signals. The
// default window (0, 255, no gate, curve 100) is exactly the stock
// AudioEffectManager::_calc_signal_value formula.
// Mirrored by scaled_audio() in esp_led_lab/src/render.cpp.
struct AudioWindow {
    uint8_t in_min = 0;
    uint8_t in_max = 255;
    bool gate = false;
    uint16_t curve = 100; // exponent x 100, 25..400

    [[nodiscard]] bool is_default() const { return in_min == 0 && in_max == 255 && !gate && curve == 100; }
};

// t^(curve / 100) for t = index / 256, scaled to 0..65535, interpolated in
// between. Rebuilt only when the exponent changes (powf per LED is too slow).
class TransferCurve {
    uint16_t _curve = 0;
    uint16_t _table[257] = {};
public:
    uint32_t apply(uint32_t t16, uint16_t curve) {
        if (curve != _curve) {
            _curve = curve;
            const float exponent = curve / 100.f;
            for (int i = 0; i <= 256; ++i)
                _table[i] = static_cast<uint16_t>(lroundf(65535.f * powf(i / 256.f, exponent)));
        }
        if (t16 >= 65535) return 65535;
        const uint32_t index = t16 >> 8, frac = t16 & 0xFF;
        return _table[index] + ((int32_t(_table[index + 1]) - int32_t(_table[index])) * int32_t(frac)) / 256;
    }
};

inline int32_t audioTransfer(uint32_t value, uint32_t max_value, int32_t count,
                             uint8_t out_min, uint8_t out_max, const AudioWindow &window = {}) {
    if (out_min > out_max) out_min = out_max;
    const int32_t count_min = int32_t(out_min) * count / 255;
    const int32_t count_max = int32_t(out_max) * count / 255;
    const int32_t actual = count_max - count_min;
    if (max_value == 0) return count_min;
    if (window.is_default()) return count_min + int32_t(int64_t(value) * actual / int32_t(max_value));

    const uint64_t scaled = uint64_t(value) * 255;
    if (scaled < uint64_t(window.in_min) * max_value) return window.gate ? 0 : count_min;
    if (scaled >= uint64_t(window.in_max) * max_value) return count_max;
    const uint64_t num = scaled - uint64_t(window.in_min) * max_value;
    const uint64_t den = uint64_t(window.in_max - window.in_min) * max_value;
    if (window.curve == 100) return count_min + int32_t(int64_t(num) * actual / int64_t(den));
    static TransferCurve curve;
    const uint32_t t16 = static_cast<uint32_t>(num * 65535 / den);
    return count_min + int32_t(int64_t(curve.apply(t16, window.curve)) * actual / 65535);
}
