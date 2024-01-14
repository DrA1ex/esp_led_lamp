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

Led led(WIDTH, HEIGHT);

Timer globalTimer;
Storage<Config> configStorage(globalTimer, 0);

AppConfig appConfig(configStorage);

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

    wifi_connect(WIFI_MODE, WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL);

    udpServer.begin(UDP_PORT);
    wsServer.begin(webServer);

    webServer.begin();

    ArduinoOTA.setHostname(MDNS_NAME);
    ArduinoOTA.begin();
}

void render();

void loop() {
    const auto start_t = millis();
    if (appConfig.config.power) {
        render();
    } else {
        led.clear();
        led.show();
    }

    globalTimer.handle_timers();

    ArduinoOTA.handle();
    wifi_check_connection();

    udpServer.handle_incoming_data();
    wsServer.handle_incoming_data();

    const auto total_t = millis() - start_t;
    const auto target_delay = 1000 / FRAMES_PER_SECOND;
    if (total_t < target_delay) {
        delay(target_delay - total_t);
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