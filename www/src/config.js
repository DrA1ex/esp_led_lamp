import {PacketType} from "./network/cmd.js";
import {BinaryParser} from "./misc/binary_parser.js";

class Preset {
    #ws;
    #config;

    current;
    list;

    get name() {
        return this.list[this.#config.presetId]?.name;
    }

    constructor(ws, config) {
        this.#ws = ws;
        this.#config = config;
    }

    async load() {
        const [cfg, list] = await Promise.all([this.#request_preset_config(), this.#request_presets()]);

        this.current = cfg;
        this.list = list;
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

export class Config {
    #ws;

    power;
    maxBrightness;
    eco;
    presetId;
    colorCorrection;
    nightMode;

    presets;

    constructor(ws) {
        this.#ws = ws;
    }


    async load() {
        this.preset = new Preset(this.#ws, this);

        const [buffer] = await Promise.all([
            this.#ws.request(PacketType.GET_CONFIG), this.preset.load()]);

        const parser = new BinaryParser(buffer);

        this.power = parser.readBoolean();

        this.maxBrightness = parser.readUInt8();
        this.eco = parser.readUInt8();

        this.presetId = parser.readUInt8();
        this.colorCorrection = parser.readUInt32();

        this.nightMode = {
            enabled: parser.readBoolean(),
            brightness: parser.readUInt8(),
            eco: parser.readUInt8(),

            startTime: parser.readUInt32(),
            endTime: parser.readUInt32(),
            switchInterval: parser.readUInt16(),
        };
    }
}