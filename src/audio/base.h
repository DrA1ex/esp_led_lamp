#pragma once

#include <cctype>

class SignalProvider {
public:
    virtual void tick() = 0;

    [[nodiscard]] virtual inline uint16_t max_value() const = 0;

    [[nodiscard]] virtual inline uint16_t get(uint16_t index, uint8_t frac) const = 0;
    [[nodiscard]] virtual inline uint16_t delta() const = 0;

    [[nodiscard]] virtual inline uint16_t update_interval() const = 0;

    virtual void set_gain(uint16_t gain) = 0;
    virtual void set_gate(uint16_t gate) = 0;

protected:
    [[nodiscard]] static inline uint16_t _calculate_read_interval(
            uint16_t update_interval, uint16_t sample_size, uint16_t sample_rate) {
        return max(0, update_interval - ((int32_t) 1000 * sample_size / sample_rate) * 2);
    }

    static void _filter_signal(uint16_t *data, size_t size, uint16_t gain, uint16_t gate) {
        if (gain == 0 && gate == 0) return;

        for (size_t i = 0; i < size; ++i) {
            auto &value = data[i];

            if (value < gate) value = 0;
            if (gain > 0) value *= gain;
        }
    }
};