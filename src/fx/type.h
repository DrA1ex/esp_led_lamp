#pragma once

#include "led.h"

struct Config;

struct FxStateBase {
    unsigned long time = 0;
    unsigned long prev_time = 0;

    [[nodiscard]] inline unsigned long delta() const { return time - prev_time; }
};

struct ColorEffectState : FxStateBase {
    struct {
        const CRGBPalette16 *palette = nullptr;
        byte scale = 0;
        byte speed = 0;
    } params;

    float current_scale_value = 0;
    float current_speed_value = 0;

    float prev_scale_value = 0;
    float prev_speed_value = 0;
};

typedef void(*ColorEffectFn)(Led &led, ColorEffectState &state);

struct BrightnessEffectState : FxStateBase {
    struct {
        byte level = 0;
    } params;

    float current_level_value = 0;
    float prev_level_value = 0;
};

typedef void (*BrightnessEffectFn)(Led &led, BrightnessEffectState &state);

enum class PaletteEnum : uint8_t {
    HEAT_COLORS,
    FIRE,
    WOOD_FIRE,
    NORMAL_FIRE,
    LITHIUM_FIRE,
    SODIUM_FIRE,
    COPPER_FIRE,
    ALCOHOL_FIRE,
    LAVA,
    PARTY,
    RAINBOW,
    RAINBOW_STRIPE,
    CLOUD,
    OCEAN,
    FOREST,
    SUNSET,
    DK_BLUE_RED,
    OPTIMUS_PRIME,
    WARM,
    COLD,
    HOT,
    PINK,
    COMFY,
    CYBERPUNK,
    GIRL,
    XMAS,
    ACID,
    BLUE_SMOKE,
    GUMMY,
    LEO,
    AURORA,
};

enum class ColorEffectEnum : uint8_t {
    PERLIN,
    GRADIENT,
    PACIFIC,
    PARTICLES,
    CHANGE_COLOR,
    SOLID,
};

enum class BrightnessEffectEnum : uint8_t {
    FIXED,
    PULSE,
    WAVE,
    DOUBLE_WAVE,
    ECO,
};


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

typedef FxConfigEntry<PaletteEnum, CRGBPalette16> PaletteEntry;
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

    void _before_call() {
        _state.time = millis();
    }

    void _after_call() {
        _state.prev_time = _state.time;
    }

    void static _save_next_value(float &prev, float &current) {
        // Avoid overflow to +INF
        if (current > MAX_SAFE_FLOAT_VALUE) {
            current -= MAX_SAFE_FLOAT_VALUE;
            D_PRINT("Resetting floating-point value due to risk of overflow");
        }

        prev = current;
    }

    uint32_t static _apply_period(float &value, uint32_t period) {
        if (value >= period) value -= period;
        return (uint32_t) value % period;
    }

    virtual void _reset_state() = 0;

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
            _reset_state();
        }
    };

    [[nodiscard]] inline const ConfigT &config() const { return _config; }
    [[nodiscard]] inline CodeT fx() const { return _fx; }
};