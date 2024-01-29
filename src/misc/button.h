#pragma once

#include <c_types.h>

typedef void (*ButtonOnClickFn)(uint8_t count);
typedef ButtonOnClickFn ButtonOnHoldFn;

class Button {
    uint8_t _pin;

    volatile bool _hold = false;
    volatile int _click_count = 0;

    unsigned long _last_impulse_time = 0;

    ButtonOnClickFn _click_handler = nullptr;
    ButtonOnHoldFn _hold_handler = nullptr;

public:
    explicit Button(uint8_t pin);

    void begin();
    void handle();

    inline void set_on_click(ButtonOnClickFn fn) { _click_handler = fn; }
    inline void set_on_hold(ButtonOnHoldFn fn) { _hold_handler = fn; }

private:
    static IRAM_ATTR void _handle_interrupt(void *arg);

    IRAM_ATTR void _handle_rising_interrupt();
    IRAM_ATTR void _handle_falling_interrupt();
};