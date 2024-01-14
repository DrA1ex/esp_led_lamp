#include <Arduino.h>
#include <ArduinoOTA.h>

#include "constants.h"
#include "config.h"
#include "led.h"
#include "storage.h"
#include "timer.h"

#include "network/wifi.h"
#include "network/web.h"
#include "network/protocol/udp.h"
#include "network/protocol/ws.h"

void render();
void render_loop(void *);
void service_loop(void *);

Led led(WIDTH, HEIGHT);

Timer globalTimer;
Storage<Config> configStorage(globalTimer, 0);

AppConfig appConfig(configStorage);

WifiManager wifiManager;
WebServer webServer(WEB_PORT);

UdpServer udpServer((AppConfig &) appConfig);
WebSocketServer wsServer((AppConfig &) appConfig);

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    delay(2000);
#endif

    configStorage.begin();
    appConfig.load();

    led.setPowerLimit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.setCorrection(TypicalLEDStrip);
    led.setBrightness(appConfig.config.maxBrightness);

    led.clear();
    led.show();

    globalTimer.add_interval(render_loop, 1000 / FRAMES_PER_SECOND);
    globalTimer.add_interval(service_loop, 20);
}


void loop() {
    globalTimer.handle_timers();
}

void render_loop(void *) {
#if DEBUG_LEVEL == __DEBUG_LEVEL_VERBOSE
    static unsigned long t = 0;
    static unsigned long ii = 0;
    if (ii % 10 == 0) D_PRINTF("Render latency: %lu\n", millis() - t);

    t = millis();
    ++ii;
#endif

    if (appConfig.config.power) {
        render();
    } else {
        led.clear();
        led.show();
    }
}


void render() {
    const auto &effectFn = appConfig.colorEffect->value;
    const auto &brightnessFn = appConfig.brightnessEffect->value;
    const auto &palette = appConfig.palette->value;

    led.clear();

    const auto &config = appConfig.config;
    led.setBrightness(config.maxBrightness);
    effectFn(led, palette, config.scale, config.speed);
    brightnessFn(led, config.light);

    led.show();
}

void service_loop(void *) {
#if DEBUG_LEVEL == __DEBUG_LEVEL_VERBOSE
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

            ArduinoOTA.setHostname(MDNS_NAME);
            ArduinoOTA.begin();

            state++;
            break;

        case 3: {
            ArduinoOTA.handle();
            wifiManager.handle_connection();

            udpServer.handle_incoming_data();
            wsServer.handle_incoming_data();
            break;
        }

        default:;
    }
}