#pragma once

#include <EEPROM.h>
#include <type_traits>

#include "constants.h"
#include "debug.h"
#include "timer.h"

template<typename T>
class Storage {
    volatile static bool _global_initialized;

    T _data;
    Timer &_timer;

    uint16_t _offset;
    uint32_t _header;
    uint8_t _version;
    long _save_timer_id = -1;

public:
    Storage(Timer &timer, uint16_t offset, uint32_t header = STORAGE_HEADER, uint8_t version = STORAGE_VERSION);

    void begin();

    inline T &get() { return _data; }
    inline const T &get() const { return _data; }
    inline uint16_t size() const { return sizeof(_header) + sizeof(_version) + sizeof(T); }
    inline Timer &timer() { return _timer; }
    inline bool is_pending_commit() const { return _save_timer_id != -1; }

    void reset();
    void save();
    void force_save();
};