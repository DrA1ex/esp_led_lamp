#pragma once

#include <Arduino.h>

#include "debug.h"

class AnalogSample {
    uint16_t *_data;
    size_t _size;
    size_t _offset;

public:
    AnalogSample(uint16_t *data, size_t size, size_t offset) : _data(data), _size(size), _offset(offset) {};

    AnalogSample(const AnalogSample &) = delete;
    AnalogSample &operator=(const AnalogSample &) = delete;

    [[nodiscard]] inline size_t size() const { return _size; }
    [[nodiscard]] inline size_t offset() const { return _offset; }

    [[nodiscard]] inline const uint16_t *data() const { return _data; }

    inline uint16_t &operator[](size_t index) { return _data[_get_index(index)]; }
    inline const uint16_t &operator[](size_t index) const { return _data[_get_index(index)]; }

private:
    [[nodiscard]] inline size_t _get_index(size_t index) const {
        const auto _pos = index + _offset;
        return _pos >= _size ? _pos - _size : _pos;
    }
};

class AnalogReader {
    size_t _size;
    uint16_t *_data;

    uint8_t _pin;
    uint16_t _sample_rate;
    uint16_t _read_interval;

    unsigned long _last_read_time = 0;
    size_t _index = 0;

public:
    explicit AnalogReader(size_t size, uint8_t pin, uint16_t sample_rate);
    ~AnalogReader();

    bool read_if_needed();

    void read_sample();

    [[nodiscard]] uint16_t *data() { return this->_rearrange(), _data; }
    [[nodiscard]] inline AnalogSample raw_data() { return {_data, _size, _index}; };

private:
    void _rearrange();
    static void _reverse(uint16_t *data, size_t left, size_t right);
};

AnalogReader::AnalogReader(size_t size, uint8_t pin, uint16_t sample_rate) : _size(size), _pin(pin), _sample_rate(sample_rate) {
    _data = new uint16_t[size];
    memset(_data, 0, sizeof(uint16_t) * size);

    _read_interval = (uint32_t) 1000000 / _sample_rate;
}

AnalogReader::~AnalogReader() {
    delete[] _data;
}

bool AnalogReader::read_if_needed() {
    const auto now = micros();
    if (now - _last_read_time < _read_interval) return false;

    _last_read_time = now;
    _data[_index] = analogRead(_pin);

    if (++_index == _size) _index = 0;

    return true;
}

void AnalogReader::read_sample() {
    _index = 0;

    for (size_t i = 0; i < _size; ++i) {
        const auto t = micros();

        _data[i] = analogRead(_pin);

        const auto elapsed = micros() - t;

        if (elapsed < _read_interval) {
            delayMicroseconds(_read_interval - elapsed);
        }
    }
}

void AnalogReader::_rearrange() {
    if (_index == 0) return;

    _reverse(_data, 0, _size - 1);
    _reverse(_data, 0, _size - _index - 1);
    _reverse(_data, _size - _index, _size - 1);

    _index = 0;
}

void AnalogReader::_reverse(uint16_t *data, size_t left, size_t right) {
    while (left < right) {
        std::swap(data[left], data[right]);

        left++;
        right--;
    }
}
