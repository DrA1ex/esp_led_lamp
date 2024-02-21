#include "storage.h"

template<typename T>
Storage<T>::Storage(Timer &timer, const char *key, uint8_t version, uint32_t header) :
        _timer(timer), _key(key), _version(version), _header(header) {}

template<typename T>
void Storage<T>::begin(FS *fs) {
    _fs = fs;

    const String path = _get_path();

    bool success = false;

    if (_fs->exists(path)) {
        auto file = _fs->open(path, "r");

        if (file.size() == size()) {
            decltype(_header) saved_header;
            decltype(_version) saved_version;

            file.read((uint8_t *) &saved_header, sizeof(_header));
            file.read((uint8_t *) &saved_version, sizeof(_version));

            if (saved_header == _header && saved_version == _version) {
                file.read((uint8_t *) &_data, sizeof(_data));
                success = true;

                D_PRINTF("Storage(%s): Loaded stored value version: %u, size %u\n", _key, saved_version, size());
            } else {
                D_PRINTF("Storage(%s): Unsupported value, expected version: %u, header: %X\n", _key, _version, _header);
            }
        } else {
            D_PRINTF("Storage(%s): Size doesn't match, expected %u, got %u\n", _key, size(), file.size());
        }

        file.close();
    } else {
        D_PRINTF("Storage(%s): Data doesn't exists\n", _key);
    }

    if (!success) {
        D_PRINTF("Storage(%s): Reset value...\n", _key);
        _data = T();
    }
}

template<typename T>
void Storage<T>::reset() {
    _data = T();
    save();
}

template<typename T>
void Storage<T>::_commit_impl() {
    if (!_fs) return;

    File file = _fs->open(_get_path(), "w+");

    if (!_check_changed(file)) {
        D_PRINTF("Storage(%s): Skip commit, data not changed\n", _key);
        return;
    }

    file.seek(0);

    file.write((uint8_t *) &_header, sizeof(_header));
    file.write((uint8_t *) &_version, sizeof(_version));
    file.write((uint8_t *) &_data, sizeof(T));

    D_PRINTF("Storage(%s): Changes committed\n", _key);

    file.close();
}

template<typename T>
bool Storage<T>::_check_header(File &file, uint32_t &out_header, uint8_t out_version) {
    file.read((uint8_t *) &out_header, sizeof(_header));
    file.read((uint8_t *) &out_version, sizeof(_version));

    return out_header == _header && out_version == _version;
}

template<typename T>
bool Storage<T>::_check_header(File &file) {
    decltype(_header) saved_header{};
    decltype(_version) saved_version{};

    return _check_header(file, saved_header, saved_version);
}

template<typename T>
void Storage<T>::save() {
    if (!_fs) return;

    if (_save_timer_id != -1) {
        D_PRINTF("Storage(%s): Clear existing save timer\n", _key);
        _timer.clear_timeout(_save_timer_id);
    }

    D_PRINTF("Storage(%s): Schedule storage commit...\n", _key);
    _save_timer_id = (long) _timer.add_timeout([](void *param) {
        auto *self = (Storage *) param;
        self->_save_timer_id = -1;
        self->_commit_impl();
    }, STORAGE_SAVE_INTERVAL, this);
}

template<typename T>
void Storage<T>::force_save() {
    if (_save_timer_id != -1) {
        D_PRINTF("Storage(%s): Clear existing Storage save timer\n", _key);

        _timer.clear_timeout(_save_timer_id);
    }

    _commit_impl();
}
