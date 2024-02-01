#pragma once

#include <EEPROM.h>
#include <type_traits>

#include "constants.h"
#include "debug.h"
#include "timer.h"

class StorageGlobal { ;
public:
    static bool storage_initialized;
};

template<typename T>
class Storage {
    T _data;
    Timer &_timer;

    uint16_t _offset;
    uint8_t _version;
    uint32_t _header;
    long _save_timer_id = -1;

public:
    Storage(Timer &timer, uint16_t offset, uint8_t version, uint32_t header = STORAGE_HEADER);

    void begin();

    [[nodiscard]] inline T &get() { return _data; }
    [[nodiscard]] inline const T &get() const { return _data; }
    [[nodiscard]] inline uint16_t size() const { return sizeof(_header) + sizeof(_version) + sizeof(T); }
    [[nodiscard]] inline uint16_t end_offset() const { return _offset + size(); }
    [[nodiscard]] inline Timer &timer() { return _timer; }
    [[nodiscard]] inline bool is_pending_commit() const { return _save_timer_id != -1; }

    void reset();
    void save();
    void force_save();
};