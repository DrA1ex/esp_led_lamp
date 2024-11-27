#include "application.h"

#include "night_mode.h"
#include "sys_macro.h"

#include "fx/fx.h"
#include "fx/palette.h"

#include "misc/ntp_time.h"
#include "utils/color.h"
#include "utils/palette.h"


Application::Application(Storage<Config> &config_storage, Storage<PresetNames> &preset_names_storage,
                         Storage<PresetConfigs> &preset_configs_storage, Storage<CustomPaletteConfig> &custom_palette_storage,
                         NightModeManager &night_mode_manager,
                         SignalProvider *wave_provider, SignalProvider *spectrum_provider, SignalProvider *parametric_provider) :
        config_storage(config_storage), preset_names_storage(preset_names_storage),
        preset_configs_storage(preset_configs_storage), custom_palette_storage(custom_palette_storage),
        config(config_storage.get()), preset_names(preset_names_storage.get()),
        preset_configs(preset_configs_storage.get()), custom_palette_config(custom_palette_storage.get()),
        night_mode_manager(night_mode_manager),
        wave_provider(wave_provider), spectrum_provider(spectrum_provider), parametric_provider(parametric_provider) {

    event_property_changed.subscribe(this, [this](auto sender, auto type, auto arg) {
        if (sender == this) return;

        if (type == NotificationProperty::COLOR && arg) {
            this->change_color(*(uint32_t *) arg);
        } else if (type == NotificationProperty::NIGHT_MODE_ENABLED) {
            this->night_mode_manager.reset();
        }

        this->load();
    });
}
void Application::begin() {
    load();

    for (int i = 0; i < preset_names.count; ++i) {
        refresh_preset_name(i);
    }
}

void Application::load() {
    const auto &preset = this->preset();
    ColorEffects.select(preset.color_effect);
    BrightnessEffects.select(preset.brightness_effect);
    AudioEffects.select(config.audio_config.effect);

    if ((int) preset.palette < Palettes.count) {
        palette = &Palettes.entries[(int) preset.palette];
    } else {
        palette = &Palettes.entries[0];
    }

    if (palette->code == PaletteEnum::CUSTOM && custom_palette_config.updated) {
        current_palette = custom_palette_config.colors;
    } else {
        set_palette(current_palette, palette->value.data, palette->value.size);
    }

    if (config.preset_id >= preset_configs.count) {
        config.preset_id = preset_configs.count - 1;
    }

#if GAMMA_CORRECTION_RT == DISABLED
    if (config.gamma != 0) {
        napplyGamma_video(current_palette.entries, 16, gamma_value(config.gamma));
    }
#endif

#if AUDIO == ENABLED
    const uint16_t gain = config.audio_config.gain * 100 / 255;
    const uint16_t gate = (int32_t) config.audio_config.gate * AUDIO_MAX_SIGNAL / 255;

    wave_provider->set_gain(gain);
    wave_provider->set_gate(gate);

    spectrum_provider->set_gain(gain);
    spectrum_provider->set_gate(gate);

    parametric_provider->set_gain(gain);
    parametric_provider->set_gate(gate);
#endif
}

void Application::update() {
    config_storage.save();
    preset_names_storage.save();
    preset_configs_storage.save();

    load();
}

void Application::refresh_preset_name(uint8 preset_id) {
    if(preset_names.custom[preset_id]) return;

    auto &preset = preset_configs.presets[preset_id];

    const char *effect_name;
    switch (preset.color_effect) {
        case ColorEffectEnum::PERLIN:
            effect_name = "Noise";
            break;
        case ColorEffectEnum::GRADIENT:
            effect_name = "Gradient";
            break;
        case ColorEffectEnum::FIRE:
            effect_name = "Fire";
            break;
        case ColorEffectEnum::AURORA:
            effect_name = "Aurora";
            break;
        case ColorEffectEnum::PLASMA:
            effect_name = "Plasma";
            break;
        case ColorEffectEnum::RIPPLE:
            effect_name = "Ripple";
            break;
        case ColorEffectEnum::VELUM:
            effect_name = "Velum";
            break;
        case ColorEffectEnum::PARTICLES:
            effect_name = "Sparkles";
            break;
        case ColorEffectEnum::CHANGE_COLOR:
            effect_name = "Colors";
            break;
        case ColorEffectEnum::SOLID:
            effect_name = "Color";
            break;
        default:
            effect_name = "Unknown";
    }

    if (preset.color_effect != ColorEffectEnum::SOLID) {
        const char *palette_name = Palettes.entries[(int) preset.palette].name;
        snprintf(preset_names.names[preset_id], preset_names.length,
            "%s %s", palette_name, effect_name);
    } else {
        snprintf(preset_names.names[preset_id], preset_names.length, "Single Color");
    }
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
    if (preset_id >= preset_configs.count) return;

    config.preset_id = preset_id;
    config_storage.save();

    D_PRINTF("Change preset: %s\n", preset_names.names[preset_id]);

    load();
}

void Application::restart() {
    if (config_storage.is_pending_commit()) config_storage.force_save();
    if (preset_names_storage.is_pending_commit()) preset_names_storage.force_save();
    if (preset_configs_storage.is_pending_commit()) preset_configs_storage.force_save();
    if (custom_palette_storage.is_pending_commit()) custom_palette_storage.force_save();

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

void Application::change_color(uint32_t color) {
    auto preset_index = preset_configs.count - 1;
    auto &preset = preset_configs.presets[preset_index];

    auto preset_name = "API Color";
    memcpy(preset_names.names[preset_index], preset_name, std::min<uint8_t>(preset_names.length, strlen(preset_name) + 1));
    preset_names.custom[preset_index] = true;

    preset.color_effect = ColorEffectEnum::SOLID;
    preset.brightness_effect = BrightnessEffectEnum::FIXED;
    config.preset_id = preset_index;

    auto rgb = CRGB(color);
    auto hsv = RGBToHSL(color);

    D_PRINTF("Change Color: RGB: #%0.2x%0.2x%0.2x, HSL: (%uº, %u%%, %u%%)\n", rgb.r, rgb.g, rgb.b,
             hsv.h * 360 / 255, hsv.s * 100 / 255, hsv.l * 100 / 255);

    preset.speed = hsv.h;
    preset.scale = hsv.s;
    preset.light = hsv.l;

    load();
}

uint32_t Application::current_color() {
    auto preset_index = preset_configs.count - 1;
    auto &preset = preset_configs.presets[preset_index];

    if (preset.color_effect == ColorEffectEnum::SOLID) {
        return HSLToRGB({preset.speed, preset.scale, preset.light});
    }

    return 0xffffff;
}

SignalProvider *Application::signal_provider() const {
    if (config.audio_config.is_spectrum()) {
        return spectrum_provider;
    } else if (config.audio_config.is_wave()) {
        return wave_provider;
    } else {
        return parametric_provider;
    }
}
