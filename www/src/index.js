import {PropertyConfig} from "./props.js";
import {Config} from "./config.js";

import {TriggerControl} from "./control/trigger.js";
import {FrameControl} from "./control/frame.js";
import {TextControl} from "./control/text.js";
import {WheelControl} from "./control/wheel.js";
import {SelectControl} from "./control/select.js";
import {InputControl, InputType} from "./control/input.js";

import {PacketType} from "./network/cmd.js";
import {WebSocketInteraction} from "./network/ws.js";
import {BinaryParser} from "./misc/binary_parser.js";

import * as FunctionUtils from "./utils/function.js"
import {ButtonControl} from "./control/button.js";

const StatusElement = document.getElementById("status");

const host = window.location.hostname;
const gateway = `ws://${host !== "localhost" ? host : "esp_lamp.local"}/ws`;

const ws = new WebSocketInteraction(gateway);

let initialized = false;

ws.subscribe(this, WebSocketInteraction.CONNECTED, async () => {
    StatusElement.style.visibility = "collapse";

    if (!initialized) {
        initialized = true;
        await initialize();
    } else {
        await refresh();
    }
});

ws.subscribe(this, WebSocketInteraction.DISCONNECTED, () => {
    StatusElement.innerText = "NOT CONNECTED";
    StatusElement.style.visibility = "visible";
});

async function request_fx(cmd) {
    const buffer = await ws.request(cmd);
    const parser = new BinaryParser(buffer);

    const count = parser.readUInt8();

    const result = new Array(count);
    for (let i = 0; i < count; i++) {
        result[i] = {
            code: parser.readUInt8(),
            name: parser.readNullTerminatedString()
        }
    }

    return result;
}

function createSelect(list, value, cmd) {
    const control = new SelectControl(document.createElement("div"));
    control.setOptions(list.map(v => ({key: v.code, label: v.name})));
    control.setValue(value);

    control.__busy = false;
    control.setOnChange(FunctionUtils.throttle(async (value, prevValue) => {
        if (control.__busy) return FunctionUtils.ThrottleDelay;

        try {
            control.__busy = true;
            await ws.request(cmd, Uint8Array.of(Number.parseInt(value)).buffer);
        } catch (e) {
            console.error("Value save error", e);
            control.select(prevValue);
        } finally {
            control.__busy = false;
        }
    }, 1000 / 60));

    return control;
}

function createNumericInput(type, value, cmd, size, valueType) {
    const control = new InputControl(document.createElement("input"), type);
    control.setValue(value);

    control.__busy = false;
    control.setOnChange(FunctionUtils.throttle(async (value, oldValue) => {
        if (control.__busy) return FunctionUtils.ThrottleDelay;
        if (!ws.connected) return;

        if (control.isValid()) {
            const data = new Uint8Array(size);
            const view = new DataView(data.buffer);

            try {
                control.__busy = true;
                control.element.setAttribute("data-saving", "true");

                view["set" + valueType](0, value, true);
                await ws.request(cmd, data.buffer);
            } catch (e) {
                console.error("Value save error", e);
                control.setValue(oldValue)
            } finally {
                control.__busy = false;
                control.element.setAttribute("data-saving", "false");
            }
        }
    }, 1000 / 60))

    return control;
}

function createTextInput(value, cmd, maxlength) {
    const control = new InputControl(document.createElement("input"), InputType.text);
    control.setValue(value);
    control.setMaxLength(maxlength);

    control.__busy = false;
    control.setOnChange(FunctionUtils.throttle(async (newValue, oldValue) => {
        if (control.__busy) return FunctionUtils.ThrottleDelay;
        if (!ws.connected) return;

        if (control.isValid()) {
            try {
                control.__busy = true;
                control.element.setAttribute("data-saving", "true");

                const response = await ws.request(cmd, new TextEncoder().encode(newValue).buffer);
                if (response !== "OK") throw new Error(`Bad response: ${response}`);
            } catch (e) {
                console.error("Value save error", e);
                control.setValue(oldValue)
            } finally {
                control.__busy = false;
                control.element.setAttribute("data-saving", "false");
            }
        }
    }, 1000 / 60));

    return control;
}

function createWheel(value, limit, cmd) {
    const control = new WheelControl(document.createElement("div"), limit);

    control.setValue(value);
    control.__busy = false;

    const _sendRequest = FunctionUtils.throttle(async (newValue, oldValue) => {
        if (control.__busy) return FunctionUtils.ThrottleDelay;
        if (!ws.connected) return;

        control.__busy = true;

        try {
            await ws.request(cmd, Uint8Array.of(newValue).buffer);
        } catch (e) {
            console.error("Value save error", e);
            control.setValue(oldValue);
        } finally {
            control.__busy = false;
        }
    }, 1000 / 60);

    control.setOnChange(_sendRequest);

    return control;
}

function createTrigger(value, cmdOn, cmdOff = null) {
    const control = new TriggerControl(document.createElement("a"));
    control.setValue(value);

    control.setOnChange(async (value, oldValue) => {
        try {
            if (cmdOff) {
                await ws.request(value ? cmdOn : cmdOff);
            } else {
                await ws.request(cmdOn, Uint8Array.of(value ? 1 : 0).buffer);
            }
        } catch (e) {
            console.error("Value save error", e);
            control.setValue(oldValue);
        }
    });

    return control;
}

function startSection(title) {
    const frame = new FrameControl(document.createElement("div"));
    frame.addClass("section");

    const sectionTitle = new TextControl(document.createElement("h3"));
    sectionTitle.setText(title);

    frame.appendChild(sectionTitle);
    document.body.appendChild(frame.element);

    return frame;
}

function createTitle(title) {
    const titleElement = new TextControl(document.createElement("p"));
    titleElement.setText(title);

    return titleElement;
}

function createButton(label, cmd) {
    const control = new ButtonControl(document.createElement("a"));
    control.setLabel(label);
    control.addClass("m-top");

    control.setOnClick(async () => {
        await ws.request(cmd);
    });

    return control;
}

function _getValue(config, prop) {
    if (!prop.key) return null;

    let value = prop.key.split(".")
        .reduce((obj, key) => obj[key], config);

    return prop.transform ? prop.transform(value) : value;
}

async function initialize() {
    const config = new Config(ws);

    const [_, palette, colorEffects, brightnessEffects] = await Promise.all([
        config.load(),
        request_fx(PacketType.PALETTE_LIST),
        request_fx(PacketType.COLOR_EFFECT_LIST),
        request_fx(PacketType.BRIGHTNESS_EFFECT_LIST),
    ]);

    console.log("Config", config);

    const Lists = {
        palette, colorEffects, brightnessEffects, presets: config.preset.list,
    };

    console.log("Lists", Lists);

    const App = {};

    for (const cfg of PropertyConfig) {
        const section = startSection(cfg.section);

        App[cfg.section] = {section, cfg, props: {}};

        for (const prop of cfg.props) {
            let title = null
            if (prop.title) {
                title = createTitle(prop.title);
                section.appendChild(title);
            }

            const value = _getValue(config, prop) ?? prop.default;

            let control = null;
            switch (prop.type) {
                case "trigger":
                    control = createTrigger(value, ...(Array.isArray(prop.cmd) ? prop.cmd : [prop.cmd]));
                    break;

                case "wheel":
                    control = createWheel(value, prop.limit, prop.cmd);
                    break;

                case "time":
                    control = createNumericInput(InputType.time, value, prop.cmd, prop.size, prop.kind);
                    break;

                case "select":
                    control = createSelect(Lists[prop.list], value, prop.cmd);
                    break;

                case "int":
                    control = createNumericInput(InputType.int, value, prop.cmd, prop.size, prop.kind);
                    break;

                case "text":
                    control = createTextInput(value, prop.cmd, prop.maxLength);
                    break;

                case "button":
                    control = createButton(prop.label, prop.cmd);
                    break;

                default:
                    console.error("Invalid prop type.", prop)
            }

            if (control) section.appendChild(control);

            App[cfg.section].props[prop.key] = {cfg: prop, title, control};
        }
    }

    window.__app = {
        App,
        Config: config,
        Lists
    };
}

async function refresh() {
    const config = window.__app.Config;
    await config.load();

    console.log("New config", config);

    for (const cfg of PropertyConfig) {
        const section = window.__app.App[cfg.section];
        for (const prop of cfg.props) {
            if (!prop.key) continue;

            const value = _getValue(config, prop) ?? prop.default;
            section.props[prop.key].control.setValue(value);
        }
    }
}

ws.begin();
window.__ws = ws;

document.addEventListener("visibilitychange", (e) => {
    console.log("Hidden", document.hidden);

    if (document.hidden) {
        ws.close();
    } else {
        ws.connect();
    }
})