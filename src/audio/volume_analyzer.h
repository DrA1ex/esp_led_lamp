#pragma once

#include <Arduino.h>

#include "./util/analog.h"
#include "./util/fourier.h"
#include "./util/resample.h"
#include "./util/scale.h"

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
class VolumeAnalyzer : public SignalProvider {
    AnalogReader _reader;
    LogScale<> _log_scale;

    uint16_t _sample_rate;
    uint16_t _update_interval;
    uint16_t _read_interval;

    uint16_t _gain;
    uint16_t _gate;

    size_t _current_index = 0;
    uint16_t _history[BucketCount] = {};

    unsigned long _last_update_time = 0;

public:
    const uint16_t MAX_VALUE = decltype(_log_scale)::MAX_VALUE;

    VolumeAnalyzer(uint16_t sample_rate, uint16_t update_interval, uint16_t window_duration, uint16_t gain = 1, uint16_t gate = 0) :
            _reader(SampleSize, AnalogPin, sample_rate),
            _log_scale(window_duration / update_interval),
            _sample_rate(sample_rate),
            _update_interval(update_interval),
            _read_interval(_calculate_read_interval(update_interval, SampleSize, _sample_rate)),
            _gain(gain),
            _gate(gate) {}

    void tick() override;

    [[nodiscard]] uint16_t max_value() const override { return MAX_VALUE; }

    [[nodiscard]] inline uint16_t get(uint16_t index, uint8_t frac) const override;
    [[nodiscard]] inline uint16_t delta() const override;

    [[nodiscard]] inline uint16_t update_interval() const override { return _update_interval; }

    void set_gain(uint16_t gain) override { _gain = gain; }
    void set_gate(uint16_t gate) override { _gate = gate; }
};

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
void VolumeAnalyzer<SampleSize, BucketCount, AnalogPin>::tick() {
    if (millis() - _last_update_time >= _read_interval) {
        _reader.read_if_needed();
    }

    if (millis() - _last_update_time >= _update_interval) {
        auto *data = _reader.data();
        _filter_signal(data, SampleSize, _gain, _gate);

        _history[_current_index] = _log_scale.amplitude(data, SampleSize);
        if (++_current_index == BucketCount) _current_index = 0;

        _last_update_time = millis();
    }
}

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
uint16_t VolumeAnalyzer<SampleSize, BucketCount, AnalogPin>::get(uint16_t index, uint8_t) const {
    return _history[(BucketCount + _current_index - index - 1) % BucketCount];
}

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
uint16_t VolumeAnalyzer<SampleSize, BucketCount, AnalogPin>::delta() const {
    return millis() - _last_update_time;
}
