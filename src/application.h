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

    void changeState(AppState s);
    void load();
    void update();

    [[nodiscard]] bool isNightTime(const NtpTime &ntpTime);
    void handleNightMode(Led &led);
};