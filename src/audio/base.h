#pragma once

#include <cctype>

class SignalProvider {
public:
    virtual void tick() = 0;

    [[nodiscard]] virtual inline uint16_t max_value() const = 0;

    [[nodiscard]] virtual inline uint16_t get(uint16_t index, uint8_t frac) const = 0;
    [[nodiscard]] virtual inline uint16_t delta() const = 0;

    [[nodiscard]] virtual inline uint16_t update_interval() const = 0;
    virtual void set_update_interval(uint16_t interval) = 0;

protected:
    [[nodiscard]] static inline uint16_t _calculate_read_interval(
            uint16_t update_interval, uint16_t sample_size, uint16_t sample_rate) {
        return max(0, update_interval - ((int32_t) 1000 * sample_size / sample_rate) * 2);
    }
};