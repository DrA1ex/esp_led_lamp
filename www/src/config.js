import {PacketType} from "./network/cmd.js";
import {BinaryParser} from "./misc/binary_parser.js";
import {EventEmitter} from "./misc/event_emitter.js";
import {Properties} from "./props.js";

class Preset {
    #ws;
    #config;

    current = null;
    list = [];

    get name() {
        return this.list[this.#config.presetId]?.name;
    }

    set name(value) {
        this.list[this.#config.presetId].name = value;
    }

    constructor(ws, config) {
        this.#ws = ws;
        this.#config = config;
    }

    async load() {
        const [cfg, list] = await Promise.all([this.#request_preset_config(), this.#request_presets()]);

        this.current = cfg;
        this.list.splice(0);
        this.list.push(...list);
    }

    async #request_preset_config() {
        const buffer = await this.#ws.request(PacketType.GET_PRESET_CONFIG);
        const parser = new BinaryParser(buffer);

        return {
            speed: parser.readUInt8(),
            scale: parser.readUInt8(),
            light: parser.readUInt8(),

            palette: parser.readUInt8(),
            colorEffect: parser.readUInt8(),
            brightnessEffect: parser.readUInt8(),
        };
    }

    async #request_presets() {
        const buffer = await this.#ws.request(PacketType.PRESET_LIST);
        const parser = new BinaryParser(buffer);

        const count = parser.readUInt8();
        const length = parser.readUInt8();

        const result = new Array(count);
        for (let i = 0; i < count; i++) {
            result[i] = {
                code: i,
                name: parser.readFixedString(length)
            }
        }

        return result;
    }
}

export class Config extends EventEmitter {
    static LOADED = "config_loaded";
    static PROPERTY_CHANGED = "config_prop_changed";

    #ws;

    power;
    maxBrightness;
    eco;
    presetId;
    colorCorrection;
    nightMode;

    constructor(ws) {
        super();

        this.#ws = ws;
        this.preset = new Preset(this.#ws, this);
    }

    async load() {
        const [buffer] = await Promise.all([
            this.#ws.request(PacketType.GET_CONFIG), this.preset.load()]);

        const parser = new BinaryParser(buffer);

        this.power = parser.readBoolean();

        this.maxBrightness = parser.readUInt8();
        this.eco = parser.readUInt8();

        this.presetId = parser.readUInt8();

        const colorCorrection = parser.readUInt32();
        this.colorCorrection = {
            r: (colorCorrection & 0xff0000) >> 16,
            g: (colorCorrection & 0xff00) >> 8,
            b: colorCorrection & 0xff
        };

        this.nightMode = {
            enabled: parser.readBoolean(),
            brightness: parser.readUInt8(),
            eco: parser.readUInt8(),

            startTime: parser.readUInt32(),
            endTime: parser.readUInt32(),
            switchInterval: parser.readUInt16(),
        };

        this.emitEvent(Config.LOADED);
    }

    getProperty(key) {
        const prop = Properties[key];
        if (!prop) {
            console.error(`Unknown property ${key}`);
            return;
        }

        const value = prop.key.split(".").reduce((obj, key) => obj[key], this);
        return (prop.transform ? prop.transform(value) : value) ?? prop.default;
    }

    setProperty(key, value) {
        if (!Properties[key]) {
            console.error(`Unknown property ${key}`);
            return;
        }

        const oldValue = this.getProperty(key);

        this.#setProperty(key, value);
        this.emitEvent(Config.PROPERTY_CHANGED, {key, value, oldValue});
    }

    #setProperty(key, value) {
        let target = this;
        const parts = key.split(".");
        for (let i = 0; i < parts.length - 1; i++) {
            target = target[parts[i]];
        }

        target[parts.at(-1)] = value;
    }
}