#pragma once

#include <cctype>
#include <deque>

enum class WindowMode : uint8_t {
    MIN,
    MAX
};

namespace __window_internal {
    typedef uint16_t window_t;

    struct WindowPair {
        size_t index;
        window_t value;
    };
}

template<WindowMode Mode>
class Window {
    using window_t = __window_internal::window_t;
    using WindowPair = __window_internal::WindowPair;

    std::deque<WindowPair> _deque;

    bool _window_populated = false;
    size_t _current_index = 0;
    window_t _current_value = 0;

    size_t _window_size;

public:
    explicit Window(size_t size);

    [[nodiscard]] inline window_t get() const { return _current_value; };

    void add(window_t value);

private:
    inline __attribute__((always_inline)) bool _mode_op(const window_t &old_value, const window_t &new_value);
};

template<WindowMode Mode>
Window<Mode>::Window(size_t size) : _window_size(size) {}

template<WindowMode Mode>
void Window<Mode>::add(window_t value) {
    if (_window_populated) {
        while (!_deque.empty() && _current_index - _deque.front().index >= _window_size) {
            _deque.pop_front();
        }
    }

    while (!_deque.empty() && _mode_op(_deque.back().value, value)) {
        _deque.pop_back();
    }

    _deque.push_back({_current_index, value});

    _current_value = _deque.front().value;
    ++_current_index;

    if (!_window_populated && _current_index == _window_size) _window_populated = true;
}


template<>
inline __attribute__((always_inline)) bool Window<WindowMode::MAX>::_mode_op(const window_t &old_value, const window_t &new_value) {
    return new_value >= old_value;
}

template<>
inline __attribute__((always_inline)) bool Window<WindowMode::MIN>::_mode_op(const window_t &old_value, const window_t &new_value) {
    return new_value <= old_value;
}