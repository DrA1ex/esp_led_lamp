#include <Arduino.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include "application.h"
#include "constants.h"
#include "config.h"
#include "night_mode.h"
#include "sys_macro.h"

#include "audio/base.h"

#include "misc/button.h"
#include "misc/led.h"
#include "misc/storage.h"
#include "misc/timer.h"
#include "misc/ntp_time.h"

#include "fx/fx.h"

#include "network/wifi.h"
#include "network/web.h"
#include "network/protocol/server/api.h"
#include "network/protocol/server/mqtt.h"
#include "network/protocol/server/udp.h"
#include "network/protocol/server/ws.h"

void initialization_animation();
void render();
void calibration();

void button_on_click(uint8_t cnt);
void button_on_hold(uint8_t cnt);

void render_loop(void *);
void service_loop(void *);
void audio_loop(void *);

Led led(WIDTH, HEIGHT);
Timer global_timer;

uint16_t XY(uint8_t x, uint8_t y) { return led.get_index(x, y); }

Storage<Config> config_storage(global_timer, "config", STORAGE_CONFIG_VERSION);
Storage<PresetNames> preset_names_storage(global_timer, "preset_names", STORAGE_PRESET_NAMES_VERSION);
Storage<PresetConfigs> preset_configs_storage(global_timer, "preset_configs", STORAGE_PRESET_CONFIG_VERSION);
Storage<CustomPaletteConfig> custom_palette_storage(global_timer, "custom_palette", STORAGE_CUSTOM_PALETTE_VERSION);

NightModeManager night_mode_manager(config_storage.get());

#if AUDIO == ENABLED

#include "audio/spectrum_analyzer.h"
#include "audio/volume_analyzer.h"

VolumeAnalyzer<AUDIO_WAVE_SAMPLE_SIZE, WIDTH, AUDIO_PIN> wave_analyzer(
        AUDIO_SAMPLE_RATE, 1000 / AUDIO_WAVE_UPDATE_RATE, AUDIO_WINDOW_DURATION,
        AUDIO_GAIN, AUDIO_GATE
);

SpectrumAnalyzer<AUDIO_SPECTRUM_SAMPLE_SIZE, WIDTH, AUDIO_PIN> spectrum_analyzer(
        AUDIO_SAMPLE_RATE, 1000 / AUDIO_SPECTRUM_UPDATE_RATE, AUDIO_WINDOW_DURATION,
        AUDIO_GAIN, AUDIO_GATE);

VolumeAnalyzer<AUDIO_PARAMETRIC_SAMPLE_RATE, WIDTH, AUDIO_PIN> parametric_analyzer(
        AUDIO_SAMPLE_RATE, 1000 / AUDIO_PARAMETRIC_UPDATE_RATE, AUDIO_WINDOW_DURATION,
        AUDIO_GAIN, AUDIO_GATE
);

SignalProvider *wave_provider = &wave_analyzer;
SignalProvider *spectrum_provider = &spectrum_analyzer;
SignalProvider *parametric_provider = &parametric_analyzer;
#else
SignalProvider *wave_provider = nullptr;
SignalProvider *spectrum_provider = nullptr;
SignalProvider *parametric_provider = nullptr;
#endif

Application app(config_storage, preset_names_storage, preset_configs_storage, custom_palette_storage, night_mode_manager,
                wave_provider, spectrum_provider, parametric_provider);

WifiManager wifi_manager;
WebServer web_server(WEB_PORT);

ApiWebServer api_server(app);
MqttServer mqtt_server(app);
UdpServer udp_server(app);
WebSocketServer ws_server(app);

NtpTime ntp_time;

Button button(BUTTON_PIN);

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    delay(2000);
#endif

    if (!LittleFS.begin()) {
        D_PRINT("Unable to initialize FS");
    }

    config_storage.begin(&LittleFS);
    preset_names_storage.begin(&LittleFS);
    preset_configs_storage.begin(&LittleFS);
    custom_palette_storage.begin(&LittleFS);

    app.begin();

    led.set_power_limit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.set_correction(app.config.color_correction);
    led.set_brightness(app.config.max_brightness);

    led.clear();
    led.show();

    BUTTON_FN(button.begin());
    BUTTON_FN(button.set_on_click(button_on_click));
    BUTTON_FN(button.set_on_hold(button_on_hold));

    global_timer.add_interval(render_loop, 1000 / FRAMES_PER_SECOND);
    global_timer.add_interval(service_loop, 20);

    AUDIO_FN(global_timer.add_interval(audio_loop, 0));

#ifdef DEBUG
    D_PRINTF("Startup Free Heap: %u\n", ESP.getFreeHeap());
    global_timer.add_interval([](void *) {
        D_PRINTF("Free Heap: %u\n", ESP.getFreeHeap());
    }, 5ul * 60 * 1000);
#endif
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

    const auto brightness_settings = app.get_brightness_settings();

    switch (app.state) {
        case AppState::NORMAL:
        case AppState::INITIALIZATION:
        case AppState::CALIBRATION:
            led.set_brightness(brightness_settings.brightness);
            break;

        case AppState::TURNING_ON: {
            uint8_t factor = ease8InOutQuad((millis() - app.state_change_time) * 255 / POWER_CHANGE_TIMEOUT);
            uint8_t brightness = (uint16_t) factor * brightness_settings.brightness / 255;
            led.set_brightness(brightness);

            if (factor == 255) app.change_state(AppState::NORMAL);
            break;
        }

        case AppState::TURNING_OFF: {
            uint8_t factor = ease8InOutQuad(255 - (millis() - app.state_change_time) * 255 / POWER_CHANGE_TIMEOUT);
            uint8_t brightness = (uint16_t) factor * brightness_settings.brightness / 255;
            led.set_brightness(brightness);

            if (factor == 0) app.change_state(AppState::NORMAL);
            break;
        }
    }

    switch (app.state) {
        case AppState::INITIALIZATION:
            initialization_animation();
            break;

        case AppState::NORMAL:
        case AppState::TURNING_ON:
        case AppState::TURNING_OFF:
            if (brightness_settings.brightness > 0 && brightness_settings.eco > 0) {
                render();
                BrightnessEffectManager::eco(led, brightness_settings.eco);
            } else {
                led.clear();
            }

            led.show();
            break;

        case AppState::CALIBRATION:
            calibration();
            break;
    }
}


void render() {
    if (app.state == AppState::NORMAL && !app.config.power) {
        led.clear();
        return;
    }


    //TODO: Refactor
    auto preset = app.preset();

#if AUDIO == ENABLED
    const auto &audio_cfg = app.config.audio_config;
    bool audio_applied = true;

    if (audio_cfg.is_parametric()) {
        const auto value = AudioEffects.calc(led, app.signal_provider(), audio_cfg.min, audio_cfg.max);

        switch (audio_cfg.effect) {
            case AudioEffectEnum::SPEED_CONTROL:
                preset.speed = value;
                break;
            case AudioEffectEnum::SCALE_CONTROL:
                preset.scale = value;
                break;
            case AudioEffectEnum::LIGHT_CONTROL:
                preset.light = value;
                break;

            default:
                audio_applied = false;
        }
    } else {
        audio_applied = false;
    }
#endif

    const auto palette = &app.current_palette;

    ColorEffects.call(led, palette, preset, app.config.gamma);
    BrightnessEffects.call(led, preset);

#if AUDIO == ENABLED
    if (!audio_applied && audio_cfg.enabled) {
        AudioEffects.call(led, app.signal_provider(), audio_cfg.min, audio_cfg.max);
    }
#endif
}

void calibration() {
    led.set_correction(app.config.color_correction);
    led.fill_solid(CRGB::White);
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
        const auto time_factor = (t / 8 + i * 4) % 256;
        const auto brightness = 255 - cubicwave8(time_factor);
        const auto color = CRGB(CRGB::Purple).scale8(brightness);

        led.set_pixel(i, 0, color);
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
            if (wifi_manager.state() == WifiManagerState::CONNECTED) state++;

            break;

        case 2:
#ifdef WEB_AUTH
            web_server.add_handler(new WebAuthHandler());
#endif

            api_server.begin(web_server);
            udp_server.begin(UDP_PORT);
            ws_server.begin(web_server);

            if constexpr (MQTT) mqtt_server.begin();

            web_server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
                char result[320] = {};

                size_t offset = snprintf(result, sizeof(result), "General:\nHeap: %u\nNow: %lu\nTime: %lu\n",
                                         ESP.getFreeHeap(), millis(), ntp_time.epoch_tz());

                offset += BrightnessEffects.debug(result + offset, sizeof(result) - offset);
                ColorEffects.debug(result + offset, sizeof(result) - offset);

                request->send_P(200, "text/plain", result);
            });

            web_server.begin(&LittleFS);

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

            if constexpr (MQTT) mqtt_server.handle_connection();
            break;
        }

        default:;
    }

    BUTTON_FN(button.handle());
}

void audio_loop(void *) {
    const auto &cfg = app.config.audio_config;
    if (!cfg.enabled) return;

    AUDIO_FN(app.signal_provider()->tick());
}

void button_on_click(uint8_t cnt) {
    switch (cnt) {
        case 1:
            app.set_power(!app.config.power);
            break;

        case 2:
            app.change_preset((app.config.preset_id + 1) % app.preset_configs.count);
            break;

        case 3:
            app.change_preset(app.config.preset_id > 0
                              ? (app.config.preset_id - 1) % app.preset_configs.count
                              : app.preset_configs.count - 1);
            break;

        case 5:
            app.restart();
            break;

        default:
            break;
    }
}

void button_on_hold(uint8_t cnt) {
    switch (cnt) {
        case 1:
            app.brightness_increase();
            break;
        case 2:
            app.brightness_decrease();
            break;

        default:
            break;
    }
}
