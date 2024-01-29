#pragma once

#include "config.h"
#include "misc/storage.h"
#include "night_mode.h"

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

    void set_power(bool on);
    void change_preset(uint8_t preset_id);

    void brightness_increase();
    void brightness_decrease();

    void restart();

    inline PresetConfig &preset() { return preset_configs.presets[config.preset_id]; }
    [[nodiscard]] BrightnessSettings get_brightness_settings() const;
};