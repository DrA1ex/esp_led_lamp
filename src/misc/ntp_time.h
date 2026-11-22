#pragma once

#include <Arduino.h>
#include <time.h>

class NtpTime {
    int32_t _tz_offset = 0;

public:
    void begin(float tz);
    void update() {} // SNTP runs asynchronously in lwIP.

    [[nodiscard]] inline bool available() const { return time(nullptr) >= 1577836800L; }

    [[nodiscard]] inline int seconds() const { return epoch_tz() % 60; }
    [[nodiscard]] inline int minutes() const { return epoch_tz() / 60 % 60; }
    [[nodiscard]] inline int hours() const { return epoch_tz() / 3600 % 24; }
    [[nodiscard]] inline int week_day() const { return (epoch_tz() / SECONDS_PER_DAY + 4) % 7; }

    [[nodiscard]] inline int32_t tz_offset() const { return _tz_offset; }

    [[nodiscard]] inline uint32_t seconds_from_midnight_tz() const { return epoch_tz() % SECONDS_PER_DAY; }
    [[nodiscard]] inline unsigned long today_tz() const { const auto now = epoch_tz(); return now - now % SECONDS_PER_DAY; }
    [[nodiscard]] inline unsigned long epoch_tz() const { const auto now = time(nullptr); return now >= 1577836800L ? now + _tz_offset : 0; }

    [[nodiscard]] tm *date() const;

    static const uint32_t SECONDS_PER_DAY;
};
