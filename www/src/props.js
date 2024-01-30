import {PacketType} from "./network/cmd.js";

export const PropertyConfig = [{
    key: "general", section: "General", props: [
        {key: "power", title: "Power", type: "trigger", cmd: [PacketType.POWER_ON, PacketType.POWER_OFF]},
        {key: "presetId", title: "Preset", type: "select", list: "presets", cmd: PacketType.PRESET_ID},
        {key: "maxBrightness", title: "Brightness", type: "wheel", limit: 255, cmd: PacketType.MAX_BRIGHTNESS},
        {key: "eco", title: "ECO", type: "wheel", limit: 255, cmd: PacketType.ECO_LEVEL},
    ],
}, {
    key: "fx", section: "FX", props: [
        {key: "preset.name", title: "Preset name", type: "text", default: "Unnamed", maxLength: 20, cmd: PacketType.PRESET_NAME},
        {key: "preset.current.palette", title: "Palette", type: "select", list: "palette", cmd: PacketType.PALETTE},
        {key: "preset.current.colorEffect", title: "Color Effect", type: "select", list: "colorEffects", cmd: PacketType.COLOR_EFFECT},
        {
            key: "preset.current.brightnessEffect", title: "Brightness Effect", type: "select", list: "brightnessEffects",
            cmd: PacketType.BRIGHTNESS_EFFECT
        },

    ],
}, {
    key: "tune", section: "Fine Tune", props: [
        {key: "preset.current.speed", title: "Speed", type: "wheel", limit: 255, cmd: PacketType.SPEED},
        {key: "preset.current.scale", title: "Scale", type: "wheel", limit: 255, cmd: PacketType.SCALE},
        {key: "preset.current.light", title: "Light", type: "wheel", limit: 255, cmd: PacketType.LIGHT},
    ],
}, {
    key: "night_mode", section: "Night Mode", props: [
        {key: "nightMode.enabled", title: "Enabled", type: "trigger", cmd: PacketType.NIGHT_MODE_ENABLED},
        {key: "nightMode.brightness", title: "Brightness", type: "wheel", limit: 255, cmd: PacketType.NIGHT_MODE_BRIGHTNESS},
        {key: "nightMode.eco", title: "ECO", type: "wheel", limit: 255, cmd: PacketType.NIGHT_MODE_ECO},
        {key: "nightMode.startTime", title: "Start Time", type: "time", size: 4, kind: "Uint32", cmd: PacketType.NIGHT_MODE_START},
        {key: "nightMode.endTime", title: "End Time", type: "time", size: 4, kind: "Uint32", cmd: PacketType.NIGHT_MODE_END},
        {
            key: "nightMode.switchInterval", title: "Switch Interval", type: "time", size: 2, kind: "Uint16",
            cmd: PacketType.NIGHT_MODE_INTERVAL
        },
    ]
}, {
    key: "calibration", section: "Calibration", props: [
        {key: "colorCorrection.r", title: "Red", type: "wheel", limit: 255, cmd: PacketType.CALIBRATION_R},
        {key: "colorCorrection.g", title: "Green", type: "wheel", limit: 255, cmd: PacketType.CALIBRATION_G},
        {key: "colorCorrection.b", title: "Blue", type: "wheel", limit: 255, cmd: PacketType.CALIBRATION_B},
    ]
}, {
    key: "action", section: "Actions", props: [
        {key: "export", type: "button", label: "Export presets"},
        {key: "import", type: "button", label: "Import presets"}
    ]
}];

export const Properties = PropertyConfig.reduce((res, section) => {
    for (const prop of section.props) {
        if (res[prop.key]) {
            console.warn(`Key ${prop.key} already exist`);
            continue;
        }

        res[prop.key] = prop;
    }

    return res;
}, {});