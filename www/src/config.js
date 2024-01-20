import {PacketType} from "./network/cmd.js";

export const PropertyConfig = [{
    section: "General", props: [
        {key: "power", title: "Power", type: "trigger", cmd: [PacketType.POWER_ON, PacketType.POWER_OFF]},
        {key: "maxBrightness", title: "Brightness", type: "wheel", limit: 255, cmd: PacketType.MAX_BRIGHTNESS},
        {key: "eco", title: "ECO", type: "wheel", limit: 255, cmd: PacketType.ECO_LEVEL},
    ],
}, {
    section: "FX", props: [
        {key: "palette", title: "Palette", type: "select", list: "palette", cmd: PacketType.PALETTE},
        {key: "colorEffect", title: "Color Effect", type: "select", list: "colorEffects", cmd: PacketType.COLOR_EFFECT},
        {key: "brightnessEffect", title: "Brightness Effect", type: "select", list: "brightnessEffects", cmd: PacketType.BRIGHTNESS_EFFECT},
    ],
}, {
    section: "Fine Tune", props: [
        {key: "speed", title: "Speed", type: "wheel", limit: 255, cmd: PacketType.SPEED},
        {key: "scale", title: "Scale", type: "wheel", limit: 255, cmd: PacketType.SCALE},
        {key: "light", title: "Light", type: "wheel", limit: 255, cmd: PacketType.LIGHT},
    ],
}, {
    section: "Night Mode", props: [
        {key: "nightMode.enabled", title: "Enabled", type: "trigger", cmd: PacketType.NIGHT_MODE_ENABLED},
        {key: "nightMode.brightness", title: "Brightness", type: "wheel", limit: 255, cmd: PacketType.NIGHT_MODE_BRIGHTNESS},
        {key: "nightMode.eco", title: "ECO", type: "wheel", limit: 255, cmd: PacketType.NIGHT_MODE_ECO},
        {key: "nightMode.startTime", title: "Start Time", type: "time", size: 4, kind: "Uint32", cmd: PacketType.NIGHT_MODE_START},
        {key: "nightMode.endTime", title: "End Time", type: "time", size: 4, kind: "Uint32", cmd: PacketType.NIGHT_MODE_END},
        {
            key: "nightMode.switchInterval",
            title: "Switch Interval",
            type: "time",
            size: 2,
            kind: "Uint16",
            cmd: PacketType.NIGHT_MODE_INTERVAL
        },
    ]
}, {
    section: "Calibration", props: [
        {
            key: "colorCorrection",
            title: "Red",
            transform: (v) => (v & 0xff0000) >> 16,
            type: "wheel",
            limit: 255,
            cmd: PacketType.CALIBRATION_R
        },
        {
            key: "colorCorrection",
            title: "Green",
            transform: (v) => (v & 0xff00) >> 8,
            type: "wheel",
            limit: 255,
            cmd: PacketType.CALIBRATION_G
        },
        {
            key: "colorCorrection",
            title: "Blue",
            transform: (v) => v & 0xff,
            type: "wheel",
            limit: 255,
            cmd: PacketType.CALIBRATION_B
        },
    ]
}]