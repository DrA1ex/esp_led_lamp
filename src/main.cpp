#include <Arduino.h>
#include <ArduinoOTA.h>

#include "application.h"
#include "constants.h"
#include "config.h"
#include "mode.h"

#include "misc/led.h"
#include "misc/storage.h"
#include "misc/timer.h"
#include "misc/ntp_time.h"

#include "fx/fx.h"

#include "network/wifi.h"
#include "network/web.h"
#include "network/protocol/udp.h"
#include "network/protocol/ws.h"

void initialization_animation();
void render();
void calibration();

void render_loop(void *);
void service_loop(void *);

Led led(WIDTH, HEIGHT);
Timer global_timer;

Storage<Config> config_storage(global_timer, 0, STORAGE_CONFIG_VERSION);
Storage<PresetNames> preset_names_storage(global_timer, config_storage.size(), STORAGE_PRESET_NAMES_VERSION);
Storage<PresetConfigs> preset_configs_storage(global_timer, preset_names_storage.size(), STORAGE_PRESET_CONFIG_VERSION);

NightModeManager night_mode_manager(led, config_storage.get());

Application app(config_storage, preset_names_storage, preset_configs_storage, night_mode_manager);

WifiManager wifi_manager;
WebServer web_server(WEB_PORT);

UdpServer udp_server(app);
WebSocketServer ws_server(app);

NtpTime ntp_time;

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    delay(2000);
#endif

    config_storage.begin();
    preset_names_storage.begin();
    preset_configs_storage.begin();

    app.load();

    led.setPowerLimit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.setCorrection(app.config.color_correction);
    led.setBrightness(app.config.max_brightness);

    led.clear();
    led.show();

    global_timer.add_interval(render_loop, 1000 / FRAMES_PER_SECOND);
    global_timer.add_interval(service_loop, 20);
}


void loop() {
    global_timer.handle_timers();
}

void render_loop(void *) {
#if defined(DEBUG) && DEBUG_LEVEL <= __DEBUG_LEVEL_VERBOSE
    static unsigned long t = 0;
    static unsigned long ii = 0;
    if (ii % 10 == 0) D_PRINTF("Render latency: %lu\n", millis() - t);

    t = millis();
    ++ii;
#endif

    led.setBrightness(app.config.max_brightness);

    switch (app.state) {
        case AppState::INITIALIZATION:
            initialization_animation();
            break;

        case AppState::NORMAL:
            render();
            break;

        case AppState::CALIBRATION:
            calibration();
            break;
    }
}


void render() {
    if (!app.config.power || app.config.max_brightness == 0) {
        led.clear();
        led.show();

        return;
    }

    const auto palette = &app.palette->value;

    led.clear();

    const auto &preset = app.preset();
    ColorEffects.call(led, palette, preset);
    BrightnessEffects.call(led, preset);

    if (night_mode_manager.is_night_time()) {
        night_mode_manager.apply_night_settings();
    } else {
        BrightnessEffectManager::eco(led, app.config.eco);
    }

    led.show();
}

void calibration() {
    led.setCorrection(app.config.color_correction);
    led.fillSolid(CRGB::White);
    led.show();

    if (millis() - app.state_change_time > CALIBRATION_TIMEOUT) {
        app.change_state(AppState::NORMAL);
    }
}

void initialization_animation() {
    if (!app.config.power) {
        led.clear();
        led.show();

        return;
    }

    led.clear();

    const auto t = millis() - app.state_change_time;
    for (int i = 0; i < led.width(); ++i) {
        const auto time_factor = (t / 8 + i * 6) % 256;
        const auto brightness = 255 - cubicwave8(time_factor);
        const auto color = CRGB(CRGB::Purple).scale8(brightness);

        led.fillColumn(i, color);
    }

    led.show();
}

void service_loop(void *) {
#if defined(DEBUG) && DEBUG_LEVEL <= __DEBUG_LEVEL_VERBOSE
    static unsigned long t = 0;
    static unsigned long ii = 0;
    if (ii % 10 == 0) D_PRINTF("Service latency: %lu\n", millis() - t);

    t = millis();
    ++ii;
#endif

    static int state = 0;
    switch (state) {
        case 0:
            wifi_manager.connect(WIFI_MODE, WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL);
            state++;

            break;

        case 1:
            wifi_manager.handle_connection();
            if (wifi_manager.state() == WifiManagerState::CONNECTED)
                state++;

            break;

        case 2:
            udp_server.begin(UDP_PORT);
            ws_server.begin(web_server);

            web_server.begin();

            ntp_time.begin(TIME_ZONE);
            ArduinoOTA.setHostname(MDNS_NAME);
            ArduinoOTA.begin();

            app.change_state(AppState::NORMAL);
            state++;
            break;

        case 3: {
            ntp_time.update();
            night_mode_manager.handle_night(ntp_time);

            ArduinoOTA.handle();
            wifi_manager.handle_connection();

            udp_server.handle_incoming_data();
            ws_server.handle_incoming_data();
            break;
        }

        default:;
    }
}