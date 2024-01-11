#include <Arduino.h>

#include "constants.h"
#include "config.h"
#include "led.h"

#include "network/wifi.h"
#include "network/udp.h"

Led led(WIDTH, HEIGHT);

Config config;
AppConfig appConfig(config);

UdpServer udpServer(appConfig);

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif

    led.setPowerLimit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.setCorrection(TypicalLEDStrip);
    led.setBrightness(config.maxBrightness);

    led.clear();
    led.show();

    wifi_connect(WIFI_MODE, WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL);
    udpServer.begin(UDP_PORT);
}

void loop() {
    wifi_check_connection();
    udpServer.handle_incoming_data();

    const auto &effectFn = appConfig.colorEffectFn;
    const auto &brightnessFn = appConfig.brightnessEffectFn;
    const auto &palette = (const CRGBPalette16 &) *appConfig.palette;

    led.clear();

    led.setBrightness(config.maxBrightness);
    effectFn(led, palette, config.scale, config.speed);
    brightnessFn(led, config.light);

    led.show();
    delay(1000 / FRAMES_PER_SECOND);
}