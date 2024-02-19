#pragma once

#include <array>
#include <cmath>

#include "debug.h"
#include "window.h"

template<uint16_t LogCnt = 256, uint16_t MaxValue = 4096>
class LogScale {
    Window<WindowMode::MAX> _window_max;
    Window<WindowMode::MIN> _window_min;

    constexpr std::array<int16_t, LogCnt> _init_log_table();

    const std::array<int16_t, LogCnt> _log_table = _init_log_table();

public:
    static constexpr uint16_t LOG_CNT = LogCnt;
    static constexpr uint16_t MAX_VALUE = MaxValue;

    explicit LogScale(size_t window) : _window_max(window), _window_min(window) {}

    void scale(uint16_t *data, size_t size);
    uint16_t amplitude(const uint16_t *data, size_t size);

private:
    std::pair<uint16_t, uint16_t> _process_data(const uint16_t *data, size_t size);
    [[nodiscard]] inline uint16_t _width() const;
};

template<uint16_t LogCnt, uint16_t MaxValue>
void LogScale<LogCnt, MaxValue>::scale(uint16_t *data, size_t size) {
    const auto start_t = micros();

    _process_data(data, size);

    const auto width = _width();
    if (width == 0) return;

    for (size_t i = 0; i < size; ++i) {
        const uint16_t rel_value = (data[i] - _window_min.get()) * LOG_CNT / width;
        data[i] = _log_table[rel_value];
    }

    VERBOSE(D_PRINTF("LogScale: %lu us\n", micros() - start_t));
}

template<uint16_t LogCnt, uint16_t MaxValue>
uint16_t LogScale<LogCnt, MaxValue>::amplitude(const uint16_t *data, size_t size) {
    const auto [v_min, v_max] = _process_data(data, size);

    const auto width = _width();
    if (width == 0) return 0;

    const auto amplitude = v_max - v_min;

    const uint16_t rel_value = amplitude * LOG_CNT / width;
    return _log_table[rel_value];
}

template<uint16_t LogCnt, uint16_t MaxValue>
std::pair<uint16_t, uint16_t> LogScale<LogCnt, MaxValue>::_process_data(const uint16_t *data, size_t size) {
    uint16_t v_max = std::numeric_limits<uint16_t>::min(), v_min = std::numeric_limits<uint16_t>::max();
    for (size_t i = 0; i < size; ++i) {
        const auto value = data[i];

        if (value < v_min) v_min = value;
        if (value > v_max) v_max = value;
    }

    VERBOSE(D_PRINTF("LogScale Input: %u..%u\n", v_min, v_max));

    _window_min.add(v_min);
    _window_max.add(v_max);

    return {v_min, v_max};
}

template<uint16_t LogCnt, uint16_t MaxValue>
inline uint16_t LogScale<LogCnt, MaxValue>::_width() const {
    const auto min = _window_min.get(), max = _window_max.get();
    VERBOSE(D_PRINTF("LogScale Window: %u..%u\n", min, max));

    return max - min;
}

template<uint16_t LogCnt, uint16_t MaxValue>
constexpr std::array<int16_t, LogCnt> LogScale<LogCnt, MaxValue>::_init_log_table() {
    std::array<int16_t, LogCnt> a{};
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = log10(1 + 9.f * i / (LogCnt - 1)) * MaxValue;
    }

    return a;
}
