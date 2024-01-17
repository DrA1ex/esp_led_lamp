#pragma once

#include "config.h"
#include "misc/storage.h"

class Application {
public:
    Storage<Config> &storage;
    Config &config;

    unsigned long state_change_time = 0;
    AppState state = AppState::INITIALIZATION;

    const PaletteEntry *palette;

    explicit Application(Storage<Config> &storage);

    void change_state(AppState s);
    void load();
    void update();

    [[nodiscard]] bool is_night_time(const NtpTime &ntp_time);
    void handle_night_mode(Led &led);
};

class NightModeManager {
    unsigned long _next_start_time = 0;
    unsigned long _next_end_time = 0;

};