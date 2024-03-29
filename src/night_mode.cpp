#include "night_mode.h"

#include "misc/ntp_time.h"

NightModeManager::NightModeManager(const Config &config) : _config(config) {}

void NightModeManager::handle_night(const NtpTime &ntp_time) {
    if (!_config.night_mode.enabled || !ntp_time.available()) return;

    if (_need_update_parameters) {
        _update_next_night_time(ntp_time);
        _need_update_parameters = false;
    }

    const auto now = ntp_time.epoch_tz();
    _update_night_flag(now);

    if (is_night_time()) {
        const uint32_t update_factor_interval = now >= _next_start_time && now <= _next_end_time
                                                ? FACTOR_UPDATE_PERIOD_MS : FACTOR_FADE_UPDATE_PERIOD_MS;

        if (millis() - _last_fade_factor_update > update_factor_interval) {
            _update_fade_factor(now);
        }
    } else if (now > _next_start_fade_time) {
        _update_next_night_time(ntp_time);
    }
}

uint8_t _smooth(uint8_t from, uint8_t to, float factor) {
    return from - ((int16_t) from - to) * factor;
}

BrightnessSettings NightModeManager::get_night_settings() const {
    if (!is_night_time()) { return {}; }

    const uint8_t brightness = _smooth(_config.max_brightness, _config.night_mode.brightness, _fade_factor);
    const uint8_t eco = _smooth(_config.eco, _config.night_mode.eco, _fade_factor);

    return {brightness, eco};
}

void NightModeManager::reset() {
    _need_update_parameters = true;
}

void NightModeManager::_update_next_night_time(const NtpTime &ntp_time) {
    const auto cfg = _config.night_mode;

    const auto start_offset = min(NtpTime::SECONDS_PER_DAY, cfg.start_time);
    const auto end_offset = min(NtpTime::SECONDS_PER_DAY, cfg.end_time);

    auto now = ntp_time.epoch_tz();
    auto start_day = ntp_time.today_tz();
    auto end_fade_time = start_day + end_offset + cfg.switch_interval;

    if (now > end_fade_time) {
        start_day += NtpTime::SECONDS_PER_DAY;
        end_fade_time += NtpTime::SECONDS_PER_DAY;
    }

    _next_start_time = start_day + start_offset;
    if (start_offset > end_offset) _next_start_time -= NtpTime::SECONDS_PER_DAY;

    _next_start_fade_time = _next_start_time - cfg.switch_interval;

    _next_end_time = end_fade_time - cfg.switch_interval;
    _next_end_fade_time = end_fade_time;

    D_PRINTF("Next night time: %lu - %lu\n", _next_start_fade_time, _next_end_fade_time);
}

void NightModeManager::_update_night_flag(unsigned long now) {
    const bool new_value = now >= _next_start_fade_time && now <= _next_end_fade_time;
    if (new_value != _is_night) D_PRINT(new_value ? "Night time begin" : "Night time end");

    _is_night = new_value;
}

void NightModeManager::_update_fade_factor(unsigned long now) {
    if (now < _next_start_time) {
        _fade_factor = (float) (now - _next_start_fade_time) / _config.night_mode.switch_interval;
    } else if (now < _next_end_time) {
        _fade_factor = 1;
    } else if (now < _next_end_fade_time) {
        _fade_factor = (float) (_next_end_fade_time - now) / _config.night_mode.switch_interval;
    } else {
        _fade_factor = 0;
    }

    _last_fade_factor_update = millis();

    VERBOSE(D_PRINTF("Factor: %.2f\n", _fade_factor));
}