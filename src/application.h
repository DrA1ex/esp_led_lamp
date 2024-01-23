#pragma once

#include "config.h"
#include "misc/storage.h"
#include "mode.h"

class Application {
public:
    Storage<Config> &config_storage;
    Storage<PresetNames> &preset_names_storage;
    Storage<PresetConfigs> &preset_configs_storage;

    Config &config;
    PresetNames &preset_names;
    PresetConfigs &preset_configs;

    NightModeManager &night_mode_manager;

    unsigned long state_change_time = 0;
    AppState state = AppState::INITIALIZATION;

    const PaletteEntry *palette = nullptr;

    explicit Application(Storage<Config> &config_storage,
                         Storage<PresetNames> &preset_names_storage,
                         Storage<PresetConfigs> &preset_configs_storage,
                         NightModeManager &night_mode_manager);

    void change_state(AppState s);
    void load();
    void update();

    inline PresetConfig &preset() { return preset_configs.presets[config.preset_id]; }
};