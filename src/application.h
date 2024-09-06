#pragma once

#include "config.h"
#include "metadata.h"
#include "misc/storage.h"
#include "misc/event_topic.h"
#include "night_mode.h"

class Application {
public:
    Storage<Config> &config_storage;
    Storage<PresetNames> &preset_names_storage;
    Storage<PresetConfigs> &preset_configs_storage;
    Storage<CustomPaletteConfig> &custom_palette_storage;

    Config &config;
    PresetNames &preset_names;
    PresetConfigs &preset_configs;
    CustomPaletteConfig &custom_palette_config;

    NightModeManager &night_mode_manager;

    SignalProvider *wave_provider;
    SignalProvider *spectrum_provider;
    SignalProvider *parametric_provider;

    unsigned long state_change_time = 0;
    AppState state = AppState::INITIALIZATION;

    const PaletteEntry *palette = nullptr;

    CRGBPalette16 current_palette = CRGBPalette16();

    EventTopic<NotificationProperty> event_property_changed;

    Application(Storage<Config> &config_storage, Storage<PresetNames> &preset_names_storage,
                Storage<PresetConfigs> &preset_configs_storage, Storage<CustomPaletteConfig> &custom_palette_storage,
                NightModeManager &night_mode_manager,
                SignalProvider *wave_provider, SignalProvider *spectrum_provider, SignalProvider *parametric_provider);

    void change_state(AppState s);
    void load();
    void update();

    void set_power(bool on);
    void change_preset(uint8_t preset_id);

    void brightness_increase();
    void brightness_decrease();

    void change_color(uint32_t color);
    uint32_t current_color();

    void restart();

    inline PresetConfig &preset() { return preset_configs.presets[config.preset_id]; }
    [[nodiscard]] BrightnessSettings get_brightness_settings() const;

    SignalProvider *signal_provider() const;
};