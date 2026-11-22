#include "ntp_time.h"

const uint32_t NtpTime::SECONDS_PER_DAY = (uint32_t) 24 * 60 * 60;

void NtpTime::begin(float tz) {
    _tz_offset = (int32_t) tz * 3600;
    configTime(0, 0, "pool.ntp.org");
}

tm *NtpTime::date() const {
    const time_t time = epoch_tz();
    return gmtime(&time);
}
