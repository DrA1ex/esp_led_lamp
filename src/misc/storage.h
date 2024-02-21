#pragma once

#include <type_traits>
#include <FS.h>

#include "debug.h"
#include "constants.h"
#include "debug.h"
#include "timer.h"

template<typename T>
class Storage {
    FS *_fs = nullptr;
    Timer &_timer;

    const char *_key;

    T _data;

    uint8_t _version;
    uint32_t _header;
    long _save_timer_id = -1;

public:
    Storage(Timer &timer, const char *key, uint8_t version, uint32_t header = STORAGE_HEADER);

    void begin(FS *fs);

    [[nodiscard]] inline T &get() { return _data; }
    [[nodiscard]] inline const T &get() const { return _data; }

    [[nodiscard]] inline uint16_t size() const { return sizeof(_header) + sizeof(_version) + sizeof(T); }

    [[nodiscard]] inline Timer &timer() { return _timer; }
    [[nodiscard]] inline bool is_pending_commit() const { return _save_timer_id != -1; }

    void reset();
    void save();
    void force_save();

private:
    [[nodiscard]] inline String _get_path() { return String(STORAGE_PATH) + _key; };
    [[nodiscard]] bool _check_header(File &file, uint32_t &out_header, uint8_t out_version);
    [[nodiscard]] bool _check_header(File &file);

    void _commit_impl();
    bool _check_changed(File &file);
};

template<typename T>
bool Storage<T>::_check_changed(File &file) {
    if (file.size() != size() || !_check_header(file)) return true;

    auto *ptr = (uint8_t *) &_data;
    uint8_t entry;

    for (size_t i = 0; i < sizeof(T); ++i, ++ptr) {
        file.read(&entry, 1);
        if (*ptr != entry) return true;
    }

    return false;
}
