#pragma once

#include <Arduino.h>

#include "base.h"

#include "util/analog.h"
#include "util/fourier.h"
#include "util/resample.h"
#include "util/scale.h"

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
class SpectrumAnalyzer : public SignalProvider {
    Fourier<SampleSize> _fourier;
    AnalogReader _reader;
    LogScale<> _log_scale;
    Resample<BucketCount, SampleSize / 2> _resample;

    uint16_t _sample_rate;
    uint16_t _update_interval;
    uint16_t _read_interval;

    uint16_t _gain;
    uint16_t _gate;

    uint16_t _spectrum_1[SampleSize / 2]{};
    uint16_t _spectrum_2[SampleSize / 2]{};

    uint16_t *_current_spectrum = _spectrum_1;
    uint16_t *_prev_spectrum = _spectrum_2;

    unsigned long _last_fft_update = 0;

public:
    static constexpr uint16_t SPECTRUM_SIZE = decltype(_fourier)::SPECTRUM_SIZE;
    static constexpr uint16_t MAX_VALUE = decltype(_log_scale)::MAX_VALUE;

    SpectrumAnalyzer(uint16_t sample_rate, uint16_t update_interval, uint16_t window_duration, uint16_t gain = 1, uint16_t gate = 0) :
            _fourier(),
            _reader(SampleSize, AnalogPin, sample_rate),
            _log_scale(window_duration / update_interval),
            _resample(sample_rate / SPECTRUM_SIZE, sample_rate),
            _sample_rate(sample_rate),
            _update_interval(update_interval),
            _read_interval(_calculate_read_interval(update_interval, SampleSize, _sample_rate)),
            _gain(gain),
            _gate(gate) {}

    void tick() override;

    [[nodiscard]] uint16_t max_value() const override { return MAX_VALUE; }

    [[nodiscard]] uint16_t get(uint16_t index, uint8_t frac) const override;
    [[nodiscard]] inline uint16_t delta() const override;

    [[nodiscard]] inline uint16_t update_interval() const override { return _update_interval; }

    void set_gain(uint16_t gain) override { _gain = gain; }
    void set_gate(uint16_t gate) override { _gate = gate; }
};

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
void SpectrumAnalyzer<SampleSize, BucketCount, AnalogPin>::tick() {
    if (millis() - _last_fft_update >= _read_interval) {
        _reader.read_if_needed();
    }

    if (millis() - _last_fft_update >= _update_interval) {
        const auto start = millis();

        uint16_t *tmp = _current_spectrum;
        _current_spectrum = _prev_spectrum;
        _prev_spectrum = tmp;

        auto *data = _reader.data();
        _filter_signal(data, SampleSize, _gain, _gate);

        _fourier.dft(data, _current_spectrum);
        _log_scale.scale(_current_spectrum, SPECTRUM_SIZE);

        _last_fft_update = start;
    }
}

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
uint16_t SpectrumAnalyzer<SampleSize, BucketCount, AnalogPin>::get(uint16_t index, uint8_t frac) const {
    uint16_t prev_index = index > 0 ? index - 1 : 0;

    uint16_t left = _resample[prev_index];
    uint16_t right = _resample[index];

    uint32_t accumulated = 0;
    uint32_t significant_cnt = 0;
    for (int j = left; j <= right; ++j) {
        const auto value = _prev_spectrum[j] - ((int32_t) _prev_spectrum[j] - _current_spectrum[j]) * frac / 255;
        accumulated += value;

        if (value >= _gate) ++significant_cnt;
    }

    if (significant_cnt > 0) accumulated /= significant_cnt;
    else accumulated = 0;

    return (uint16_t) min<uint32_t>(accumulated, MAX_VALUE);
}

template<uint16_t SampleSize, uint16_t BucketCount, uint8_t AnalogPin>
uint16_t SpectrumAnalyzer<SampleSize, BucketCount, AnalogPin>::delta() const {
    return millis() - _last_fft_update;
}
