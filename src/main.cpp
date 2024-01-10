#include <Arduino.h>

#include "constants.h"
#include "config.h"
#include "led.h"

Led led(WIDTH, HEIGHT);
Config config;
AppConfig appConfig(config);

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif

    led.setPowerLimit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.setCorrection(TypicalLEDStrip);
    led.setBrightness(config.maxBrightness);

    led.clear();
    led.show();
}

void loop() {
    const auto &effectFn = appConfig.colorEffectFn;
    const auto &brightnessFn = appConfig.brightnessEffectFn;
    const auto &palette = *appConfig.palette;

    led.clear();

    effectFn(led, palette, config.scale, config.speed);
    brightnessFn(led, config.light);

    led.show();
    delay(1000 / FRAMES_PER_SECOND);
}