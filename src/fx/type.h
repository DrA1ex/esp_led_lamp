#pragma once

#include <type_traits>
#include <vector>

#include "constants.h"
#include "debug.h"

#include "fx/enum.h"

struct Config;
class Led;

typedef CRGBPalette16 PaletteT;

typedef uint32_t PaletteColorT;
typedef PaletteColorT PaletteDataT[];

struct FxStateBase {
    unsigned long time = 0;
    unsigned long prev_time = 0;

    double prev_time_factor = 0;
    double current_time_factor = 0;

    [[nodiscard]] inline unsigned long delta() const { return min(1000ul, time - prev_time); }
};

struct ColorEffectState : FxStateBase {
    struct {
        const PaletteT *palette = nullptr;
        byte scale = 0;
        byte speed = 0;

        bool gamma_correction = false;
        float gamma = 0;
    } params;
};

typedef void(*ColorEffectFn)(Led &led, ColorEffectState &state);

struct BrightnessEffectState : FxStateBase {
    struct {
        byte level = 0;
    } params;
};

typedef void (*BrightnessEffectFn)(Led &led, BrightnessEffectState &state);

template<typename C, typename V>
struct FxConfigEntry {
    static_assert(std::is_enum_v<C> && std::is_same_v<std::underlying_type_t<C>, uint8_t>);

    C code;
    const char *name;
    V value;
};

template<typename E>
struct FxConfig {
    using EntryT = E;

    uint8_t count = 0;
    std::vector<EntryT> entries;
};

struct PaletteData {
    const PaletteColorT *data;
    uint8_t size;

    template<unsigned int Size>
    PaletteData(const PaletteColorT (&data)[Size]): data(data), size(Size) {}
};

typedef FxConfigEntry<PaletteEnum, PaletteData> PaletteEntry;
typedef FxConfig<PaletteEntry> PaletteConfig;

typedef FxConfigEntry<ColorEffectEnum, ColorEffectFn> ColorEffectEntry;
typedef FxConfig<ColorEffectEntry> ColorEffectConfig;

typedef FxConfigEntry<BrightnessEffectEnum, BrightnessEffectFn> BrightnessEffectEntry;
typedef FxConfig<BrightnessEffectEntry> BrightnessEffectConfig;

template<typename ConfigT, typename StateT>
class FxManagerBase {
    using EntryT = typename ConfigT::EntryT;
    using CodeT = decltype(EntryT::code);

protected:
    ConfigT _config;
    CodeT _fx = (CodeT) 0;
    StateT _state;

    uint16_t _execution_time;

    void _before_call() {
        _state.prev_time = _state.time;
        _state.time = millis();

        _save_next_value(_state.prev_time_factor, _state.current_time_factor);
    }

    void _after_call() {
        _execution_time = millis() - _state.time;
    }

    void static _save_next_value(double &prev, double &current) {
        // Avoid overflow to +INF
        if (current > MAX_SAFE_FLOAT_VALUE) {
            current -= MAX_SAFE_FLOAT_VALUE;
            D_PRINT("Resetting floating-point value due to risk of overflow");
        }

        prev = current;
    }

    uint64_t static apply_period(double &value, uint64_t period) {
        if (value >= period) value -= period;
        return (uint64_t) round(value) % period;
    }

    virtual void reset_state() {
        _state.prev_time_factor = 0;
        _state.current_time_factor = 0;
        random16_add_entropy(micros() % (1 << 16));
    };

public:
    void select(CodeT fx) {
        CodeT next_fx;
        if ((int) fx < _config.count) {
            next_fx = fx;
        } else {
            next_fx = (CodeT) 0;
        }

        if (_fx != next_fx) {
            _fx = next_fx;
            reset_state();
        }
    };

    [[nodiscard]] inline const ConfigT &config() const { return _config; }
    [[nodiscard]] inline CodeT fx() const { return _fx; }

    virtual size_t debug(char *dst, size_t length);
};

template<typename ConfigT, typename StateT>
size_t FxManagerBase<ConfigT, StateT>::debug(char *dst, size_t length) {
    return snprintf(dst, length, "prev_time: %lu\ntime: %lu\ndelta: %lu\nprev_time_f: %f\ncurrent_time_f: %f\ncycle: %u\n",
                    _state.prev_time, _state.time, _state.delta(), _state.prev_time_factor, _state.current_time_factor, _execution_time);
}

template<typename T>
constexpr bool check_entry_order(const std::initializer_list<T> &lst) {
    int index = 0;
    for (const auto &v: lst) {
        if ((int) v.code != index++) {
            return false;
        }
    }

    return true;
}