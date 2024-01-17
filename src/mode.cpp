#include "mode.h"

#include "fx/brightness_effect.h"
#include "misc/led.h"
#include "misc/ntp_time.h"

NightModeManager::NightModeManager(Led &led, const Config &config) : _led(led), _config(config) {}

void NightModeManager::handle_night(const NtpTime &ntp_time) {
    if (!_config.nightMode.enabled || !ntp_time.available()) return;

    const auto now = ntp_time.epoch_tz();
    _update_night_flag(now);

    if (is_night_time()) {
        const int32_t update_factor_interval = now >= _next_start_time && now <= _next_end_time
                                               ? FACTOR_UPDATE_PERIOD_MS : FACTOR_FADE_UPDATE_PERIOD_MS;

        if (millis() - _last_fade_factor_update > update_factor_interval) {
            _update_fade_factor(now);
        }
    } else if (now > _next_start_fade_time) {
        _update_next_night_time(ntp_time);
    }
}

uint8_t _smooth(uint8_t a, uint8_t b, float factor) {
    return a - (a - b) * factor;
}

void NightModeManager::apply_night_settings() {
    const uint8_t brightness = _smooth(_config.maxBrightness, _config.nightMode.brightness, _fade_factor);
    const uint8_t eco = _smooth(_config.eco, _config.nightMode.eco, _fade_factor);

    _led.setBrightness(brightness);
    BrightnessEffectManager::eco(_led, eco);
}

void NightModeManager::reset() {
    _next_start_fade_time = 0;
    _next_start_time = 0;

    _next_end_time = 0;
    _next_end_fade_time = 0;

    _is_night = false;

    _last_fade_factor_update = 0;
    _fade_factor = 0;
}

void NightModeManager::_update_next_night_time(const NtpTime &ntp_time) {
    const auto cfg = _config.nightMode;

    const auto start_offset = min(NtpTime::SECONDS_PER_DAY, cfg.startTime);
    const auto end_offset = min(NtpTime::SECONDS_PER_DAY, cfg.endTime);

    auto now = ntp_time.epoch_tz();
    auto start_day = ntp_time.today_tz();
    auto end_fade_time = start_day + end_offset + cfg.switchInterval;

    if (now > end_fade_time) {
        start_day += NtpTime::SECONDS_PER_DAY;
        end_fade_time += NtpTime::SECONDS_PER_DAY;
    }

    _next_start_time = start_day + start_offset;
    if (start_offset > end_offset) _next_start_time -= NtpTime::SECONDS_PER_DAY;

    _next_start_fade_time = _next_start_time - cfg.switchInterval;

    _next_end_time = end_fade_time - cfg.switchInterval;
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
        _fade_factor = (float) (_next_start_time - now) / _config.nightMode.switchInterval;
    } else if (now < _next_end_time) {
        _fade_factor = 1;
    } else if (now < _next_end_fade_time) {
        _fade_factor = (float) (now - _next_end_time) / _config.nightMode.switchInterval;
    } else {
        _fade_factor = 0;
    }

    _last_fade_factor_update = millis();

    VERBOSE(D_PRINTF("Factor: %.2f\n", _fade_factor));
}