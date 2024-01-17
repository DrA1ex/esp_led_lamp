#include "ntp_time.h"

#include "constants.h"

NtpTime::NtpTime() : _ntp_client(_wifi_udp) {}

void NtpTime::begin() {
    _ntp_client.begin();
    _ntp_client.setTimeOffset(TIME_ZONE * 3600);
}

void NtpTime::update() {
    _ntp_client.update();
}

tm *NtpTime::date() const {
    const time_t time = epoch();
    return gmtime(&time);
}
