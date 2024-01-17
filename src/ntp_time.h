#pragma once

#include <WiFiUdp.h>
#include <NTPClient.h>

class NtpTime {
    WiFiUDP _wifi_udp;
    NTPClient _ntp_client;

public:
    NtpTime();

    void begin();
    void update();

    [[nodiscard]] inline bool available() const { return _ntp_client.isTimeSet(); }

    [[nodiscard]] inline int seconds() const { return _ntp_client.getSeconds(); }
    [[nodiscard]] inline int minutes() const { return _ntp_client.getMinutes(); }
    [[nodiscard]] inline int hours() const { return _ntp_client.getHours(); }
    [[nodiscard]] inline int week_day() const { return _ntp_client.getDay(); }

    [[nodiscard]] inline uint32_t seconds_from_midnight() const { return seconds() + minutes() * 60 + hours() * 3600; }
    [[nodiscard]] inline unsigned long today() const { return epoch() - seconds_from_midnight(); }

    [[nodiscard]] inline unsigned long epoch() const { return _ntp_client.getEpochTime(); }

    [[nodiscard]] tm *date() const;

    static const uint32_t SECONDS_PER_DAY = (uint32_t) 24 * 60 * 60;
};