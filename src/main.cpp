#include <Arduino.h>
#include <ArduinoOTA.h>

#include "application.h"
#include "constants.h"
#include "config.h"
#include "led.h"
#include "storage.h"
#include "timer.h"
#include "ntp_time.h"

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

Timer globalTimer;
Storage<Config> configStorage(globalTimer, 0);

Application app(configStorage);

WifiManager wifiManager;
WebServer webServer(WEB_PORT);

UdpServer udpServer(app);
WebSocketServer wsServer(app);

NtpTime ntpTime;

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    delay(2000);
#endif

    configStorage.begin();
    app.load();

    led.setPowerLimit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.setCorrection(app.config.colorCorrection);
    led.setBrightness(app.config.maxBrightness);

    led.clear();
    led.show();

    globalTimer.add_interval(render_loop, 1000 / FRAMES_PER_SECOND);
    globalTimer.add_interval(service_loop, 20);
}


void loop() {
    globalTimer.handle_timers();
}

void render_loop(void *) {
#if defined(DEBUG) && DEBUG_LEVEL <= __DEBUG_LEVEL_VERBOSE
    static unsigned long t = 0;
    static unsigned long ii = 0;
    if (ii % 10 == 0) D_PRINTF("Render latency: %lu\n", millis() - t);

    t = millis();
    ++ii;
#endif

    led.setBrightness(app.config.maxBrightness);

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
    if (!app.config.power) {
        led.clear();
        led.show();

        return;
    }

    const auto palette = &app.palette->value;

    led.clear();

    const auto &config = app.config;
    ColorEffects.call(led, palette, config);
    BrightnessEffects.call(led, config);

    const auto &nightMode = config.nightMode;
    bool nightTime = app.isNightTime(ntpTime);

    if (nightTime) {
        app.handleNightMode(led);
    } else {
        BrightnessEffectManager::eco(led, config.eco);
    }

    led.show();
}

void calibration() {
    led.setCorrection(app.config.colorCorrection);
    led.fillSolid(CRGB::White);
    led.show();

    if (millis() - app.state_change_time > CALIBRATION_TIMEOUT) {
        app.changeState(AppState::NORMAL);
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
            wifiManager.connect(WIFI_MODE, WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL);
            state++;

            break;

        case 1:
            wifiManager.handle_connection();
            if (wifiManager.state() == WifiManagerState::CONNECTED)
                state++;

            break;

        case 2:
            udpServer.begin(UDP_PORT);
            wsServer.begin(webServer);

            webServer.begin();

            ntpTime.begin();
            ArduinoOTA.setHostname(MDNS_NAME);
            ArduinoOTA.begin();

            app.changeState(AppState::NORMAL);
            state++;
            break;

        case 3: {
            ntpTime.update();

            ArduinoOTA.handle();
            wifiManager.handle_connection();

            udpServer.handle_incoming_data();
            wsServer.handle_incoming_data();
            break;
        }

        default:;
    }
}