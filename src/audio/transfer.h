#pragma once

#include <stdint.h>

// Signal window applied before the AudioConfig min/max mapping. The input
// window [in_min, in_max] (0..255 of the provider range) is stretched onto
// [out_min, out_max]; below in_min yields out_min (or 0 with gate), above
// in_max yields out_max. The default window (0, 255, no gate) is exactly the
// stock AudioEffectManager::_calc_signal_value formula.
// Mirrored by scaled_audio() in esp_led_lab/src/render.cpp.
struct AudioWindow {
    uint8_t in_min = 0;
    uint8_t in_max = 255;
    bool gate = false;

    [[nodiscard]] bool is_default() const { return in_min == 0 && in_max == 255 && !gate; }
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
    return count_min + int32_t(int64_t(num) * actual / int64_t(den));
}
