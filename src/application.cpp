#include "application.h"

#include "misc/led.h"
#include "misc/ntp_time.h"

#include "fx/fx.h"

#include "night_mode.h"

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
        palette = &Palettes.entries[0];
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

BrightnessSettings Application::get_brightness_settings() const {
    if (night_mode_manager.is_night_time()) {
        return night_mode_manager.get_night_settings();
    } else {
        return {config.max_brightness, config.eco};
    }
}

void Application::set_power(bool on) {
    config.power = on;

    if (state != AppState::INITIALIZATION) {
        change_state(config.power ? AppState::TURNING_ON : AppState::TURNING_OFF);
    }

    D_PRINTF("Turning Power: %s\n", config.power ? "ON" : "OFF");

    config_storage.save();
}

void Application::change_preset(uint8_t preset_id) {
    if (preset_id >= preset_names.count) return;

    config.preset_id = preset_id;
    config_storage.save();

    D_PRINTF("Change preset: %s\n", preset_names.names[preset_id]);

    load();
}

void Application::restart() {
    if (config_storage.is_pending_commit()) config_storage.force_save();
    if (preset_names_storage.is_pending_commit()) preset_names_storage.force_save();
    if (preset_configs_storage.is_pending_commit()) preset_configs_storage.force_save();

    D_PRINT("Restarting");

    ESP.restart();
}

void Application::brightness_increase() {
    if (config.max_brightness == 255) return;

    if (!config.power) config.power = true;

    config.max_brightness = min(255, config.max_brightness + max(1, config.max_brightness / 12));
    if (!config_storage.is_pending_commit()) config_storage.save();

    D_PRINT("Increase brightness");
}

void Application::brightness_decrease() {
    if (config.max_brightness == 0) return;

    config.max_brightness = max(0, config.max_brightness - max(1, config.max_brightness / 12));
    if (!config_storage.is_pending_commit()) config_storage.save();

    D_PRINT("Decrease brightness");
}
