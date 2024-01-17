#include "application.h"

#include "misc/led.h"
#include "misc/ntp_time.h"

#include "fx/fx.h"

#include "mode.h"

Application::Application(Storage<Config> &storage, NightModeManager &night_mode_manager) :
        storage(storage), config(storage.get()), night_mode_manager(night_mode_manager) {}

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
