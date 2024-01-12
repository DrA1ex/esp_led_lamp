#include "storage.h"

#include "Arduino.h"
#include "spi_flash_geometry.h"

#include "debug.h"

template<typename T>
volatile bool Storage<T>::_global_initialized = false;

template<typename T>
Storage<T>::Storage(Timer &timer, uint16_t offset, uint32_t header, uint8_t version) :
        _timer(timer), _offset(offset), _header(header), _version(version) {}

template<typename T>
void Storage<T>::begin() {
    if (!Storage::_global_initialized) {
        Storage::_global_initialized = true;
        EEPROM.begin(FLASH_SECTOR_SIZE);

        D_PRINT("EEPROM initialized");
    }

    const auto header_size = sizeof(_header) + sizeof(_version);

    uint32_t saved_header = 0;
    uint8_t saved_version = 0;
    EEPROM.get(_offset, saved_header);
    EEPROM.get(_offset + sizeof(_header), saved_version);

    if (saved_header == _header && saved_version == _version) {
        EEPROM.get(_offset + header_size, _data);
        D_PRINTF("Loaded stored settings version: %u\n", saved_version);
    } else {
        D_PRINTF("Unsupported settings, expected version: %u, header: %X\n", _version, _header);
        D_PRINT("Reset settings...");

        _data = T();
    }
}

template<typename T>
void Storage<T>::reset() {
    _data = T();
    save();
}

template<typename T>
void _storage_commit_impl(int offset, uint32_t header, uint8_t version, const T &data) {
    const auto header_size = sizeof(header) + sizeof(version);

    EEPROM.put(offset, header);
    EEPROM.put(offset + (int) sizeof(header), version);
    EEPROM.put(offset + header_size, data);

    auto success = EEPROM.commit();
    if (success)
        D_PRINT("EEPROM committed");
    else
        D_PRINT("EEPROM commit failed");
}

template<typename T>
void Storage<T>::save() {
    if (_save_timer_id != -1) {
        D_PRINT("Clear existing Settings save timer");
        _timer.clear_timeout(_save_timer_id);
    }

    D_PRINT("Schedule storage commit...");
    _save_timer_id = (long) _timer.add_timeout([](void *param) {
        auto *self = (Storage *) param;
        self->_save_timer_id = -1;
        _storage_commit_impl(self->_offset, self->_header, self->_version, self->_data);
    }, STORAGE_SAVE_INTERVAL, this);
}

template<typename T>
void Storage<T>::force_save() {
    if (_save_timer_id != -1) {
        D_PRINT("Clear existing Storage save timer");

        _timer.clear_timeout(_save_timer_id);
    }

    _storage_commit_impl(_offset, _header, _version, _data);
}
