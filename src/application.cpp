#include "application.h"

#include "misc/led.h"
#include "misc/ntp_time.h"

#include "fx/fx.h"

#include "mode.h"

Application::Application(Storage<Config> &config_storage,
                         Storage<PresetNames> &preset_names_storage,
                         Storage<PresetConfigs> &preset_configs_storage,
                         NightModeManager &night_mode_manager) :
        config_storage(config_storage), preset_names_storage(preset_names_storage), preset_configs_storage(preset_configs_storage),
        config(config_storage.get()), preset_names(preset_names_storage.get()), preset_configs(preset_configs_storage.get()),
        night_mode_manager(night_mode_manager) {}

void Application::load() {
    const auto &preset = this->preset();
    ColorEffects.select(preset.color_effect);
    BrightnessEffects.select(preset.brightness_effect);

    if ((int) preset.palette < Palettes.count) {
        palette = &Palettes.entries[(int) preset.palette];
    } else {
        palette = &Palettes.entries[(int) PaletteEnum::RAINBOW];
    }
}

void Application::update() {
    config_storage.save();
    preset_names_storage.save();
    preset_configs_storage.save();

    load();
}

void Application::change_state(AppState s) {
    state_change_time = millis();
    state = s;
}
