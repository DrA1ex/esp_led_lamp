#include "application.h"

#include "misc/led.h"
#include "misc/ntp_time.h"

#include "fx/fx.h"

Application::Application(Storage<Config> &storage) : storage(storage), config(storage.get()) {}

void Application::load() {
    ColorEffects.select(config.colorEffect);
    BrightnessEffects.select(config.brightnessEffect);

    if ((int) config.palette < Palettes.count) {
        palette = &Palettes.entries[(int) config.palette];
    } else {
        palette = &Palettes.entries[(int) PaletteEnum::RAINBOW];
    }
}

void Application::update() {
    storage.save();
    load();
}

void Application::change_state(AppState s) {
    state_change_time = millis();
    state = s;
}

bool Application::is_night_time(const NtpTime &ntp_time) {
    const auto &nightMode = config.nightMode;
    if (!nightMode.enabled) return false;

    const auto now = ntp_time.epoch();
    const auto md_secs = ntp_time.seconds_from_midnight();
    const auto today = now - md_secs;

    unsigned long start_day = today;
    unsigned long start_time = start_day + nightMode.startTime - nightMode.switchInterval;
    unsigned long end_time = start_day + nightMode.endTime + nightMode.switchInterval;

    if (now < start_time) {
        start_day -= NtpTime::SECONDS_PER_DAY;
        start_time -= NtpTime::SECONDS_PER_DAY;
        end_time -= NtpTime::SECONDS_PER_DAY;
    }

    return now > start_time && now < end_time;
}

void Application::handle_night_mode(Led &led) {
    const auto &nightMode = config.nightMode;

    led.setBrightness(nightMode.brightness);
    BrightnessEffectManager::eco(led, nightMode.eco);
}
