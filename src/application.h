#pragma once

#include "config.h"
#include "misc/storage.h"
#include "mode.h"

class Application {
public:
    Storage<Config> &storage;
    Config &config;
    NightModeManager &night_mode_manager;

    unsigned long state_change_time = 0;
    AppState state = AppState::INITIALIZATION;

    const PaletteEntry *palette;

    explicit Application(Storage<Config> &storage, NightModeManager &night_mode_manager);

    void change_state(AppState s);
    void load();
    void update();
};