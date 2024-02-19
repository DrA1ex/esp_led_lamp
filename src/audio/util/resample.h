#pragma once

#include <array>
#include <cctype>
#include <cmath>

template<uint16_t BucketCount, uint16_t InputSize>
class Resample {
    constexpr std::array<uint16_t, BucketCount> _init_index_table(uint16_t min_freq, uint16_t max_freq);
    const std::array<uint16_t, BucketCount> _index_table;

public:
    Resample(uint16_t min_freq, uint16_t max_freq) : _index_table(_init_index_table(min_freq, max_freq)) {}

    inline const uint16_t &operator[](size_t index) const {
        return _index_table[index];
    }
};

template<uint16_t BucketCount, uint16_t InputSize>
constexpr std::array<uint16_t, BucketCount> Resample<BucketCount, InputSize>::_init_index_table(uint16_t min_freq, uint16_t max_freq) {
    std::array<uint16_t, BucketCount> bucket_index{};
    bucket_index[0] = 0;

    float octave_step = pow(max_freq / min_freq, 1.f / BucketCount);

    D_PRINTF("Resample setup: [%u, %u] (step: %.2f)\n", min_freq, max_freq, octave_step);

    float prev_bucket_value = min_freq;
    for (int i = 1; i < BucketCount - 1; ++i) {
        float value = prev_bucket_value * octave_step;
        bucket_index[i] = floor(InputSize * value / max_freq);
        prev_bucket_value = value;

        D_PRINTF("- %u: %.2f Hz (%u)\n", i, value, bucket_index[i]);
    }

    bucket_index[BucketCount - 1] = InputSize - 1;

    return bucket_index;
}