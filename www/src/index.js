import {Properties, PropertyConfig} from "./props.js";
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

import {DEFAULT_ADDRESS, THROTTLE_INTERVAL} from "./constants.js";

const StatusElement = document.getElementById("status");

const params = new Proxy(new URLSearchParams(window.location.search), {
    get: (searchParams, prop) => searchParams.get(prop),
});

const host = params.host ?? window.location.hostname;
const gateway = `ws://${host !== "localhost" ? host : DEFAULT_ADDRESS}/ws`;

window.__app = {};
initUi();

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

function initUi() {
    const Sections = {};
    const Properties = {};

    for (const cfg of PropertyConfig) {
        const section = startSection(cfg.section);

        Sections[cfg.key] = {section, cfg, props: {}};

        for (const prop of cfg.props) {
            let title = null
            if (prop.title) {
                title = createTitle(prop.title);
                section.appendChild(title);
            }

            let control = null;
            switch (prop.type) {
                case "trigger":
                    control = new TriggerControl(document.createElement("a"));
                    break;

                case "wheel":
                    control = new WheelControl(document.createElement("div"), prop.limit);
                    break;

                case "time":
                    control = new InputControl(document.createElement("input"), InputType.time);
                    break;

                case "select":
                    control = new SelectControl(document.createElement("div"));
                    break;

                case "int":
                    control = control = new InputControl(document.createElement("input"), InputType.int);
                    break;

                case "text":
                    control = new InputControl(document.createElement("input"), InputType.text);
                    control.setMaxLength(prop.maxLength ?? 255);
                    break;

                case "button":
                    control = new ButtonControl(document.createElement("a"));
                    control.addClass("m-top");
                    control.setLabel(prop.label);
                    break;

                default:
                    console.error("Invalid prop type.", prop)
            }

            if (control) {
                control.element.setAttribute("data-loading", "true");
                section.appendChild(control);
            }

            const entry = {prop, title, control};
            Sections[cfg.key].props[prop.key] = entry;
            Properties[prop.key] = entry;
        }
    }

    window.__app.Sections = Sections;
    window.__app.Properties = Properties;
}

async function initialize() {
    const config = new Config(ws);

    window.__app.Config = config;

    const [_, palette, colorEffects, brightnessEffects] = await Promise.all([
        config.load(),
        request_fx(PacketType.PALETTE_LIST),
        request_fx(PacketType.COLOR_EFFECT_LIST),
        request_fx(PacketType.BRIGHTNESS_EFFECT_LIST),
    ]);

    const Lists = {
        palette, colorEffects, brightnessEffects, presets: config.preset.list,
    };


    console.log("Config", config);
    console.log("Lists", Lists);

    window.__app.Lists = Lists;

    config.subscribe(this, Config.LOADED, refreshConfig);
    config.subscribe(this, Config.PROPERTY_CHANGED, onConfigPropChanged);

    refreshConfig();
}

function refreshConfig() {
    const config = window.__app.Config;
    const lists = window.__app.Lists;

    for (const cfg of PropertyConfig) {
        const section = window.__app.Sections[cfg.key];
        for (const prop of cfg.props) {
            if (!prop.key) continue;

            const control = section.props[prop.key].control;

            if (prop.type === "select") {
                control.setOptions(lists[prop.list].map(v => ({key: v.code, label: v.name})));
            }

            const value = config.getProperty(prop.key);
            control.setValue(value);
            if (control.element.getAttribute("data-loading") === "true") {
                if ("setOnChange" in control) control.setOnChange((value) => config.setProperty(prop.key, value));
                else console.warn("Unsupported control type", console);

                control.element.setAttribute("data-loading", "false");
            }
        }
    }
}

async function onConfigPropChanged(config, {key, value, oldValue}) {
    const prop = Properties[key];
    if (!prop) {
        console.error(`Unknown property ${key}`);
        return;
    }

    if (value !== oldValue) {
        await sendChanges(config, prop, value, oldValue);
    }
}

const sendChanges = FunctionUtils.throttle(async function (config, prop, value, oldValue) {
    if (prop.__busy) {
        return FunctionUtils.ThrottleDelay;
    }

    const control = window.__app.Properties[prop.key].control;
    prop.__busy = true;
    try {
        if (prop.type !== "wheel") control.element.setAttribute("data-saving", "true");

        if (Array.isArray(prop.cmd)) {
            await window.__ws.request(value ? prop.cmd[0] : prop.cmd[1]);
        } else if (prop.type === "text") {
            await window.__ws.request(prop.cmd, new TextEncoder().encode(value).buffer);
        } else {
            const size = prop.size ?? 1;
            const kind = prop.kind ?? "Uint8";

            const req = new Uint8Array(size);
            const view = new DataView(req.buffer);
            view[`set${kind}`](0, value, true);

            await window.__ws.request(prop.cmd, req.buffer);
        }

        if (prop.key === "preset.name") {
            window.__app.Properties["presetId"].control.updateOption(config.presetId, value);
        }

        if (prop.key === "presetId") {
            await config.preset.loadPresetConfig();
            refreshConfig();
        }

        console.log(`Changed '${prop.key}': '${oldValue}' -> '${value}'`);
    } catch (e) {
        console.error("Unable to save changes", e);
        control.setValue(oldValue);
    } finally {
        prop.__busy = false;
        if (prop.type !== "wheel") control.element.setAttribute("data-saving", "false");
    }
}, THROTTLE_INTERVAL);

async function refresh() {
    const config = window.__app.Config;
    await config.load();

    console.log("New config", config);
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