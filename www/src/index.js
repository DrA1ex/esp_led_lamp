import {PropertyConfig} from "./config.js";
import {PacketType} from "./network/cmd.js";
import {WebSocketInteraction} from "./network/ws.js";
import {BinaryParser} from "./misc/binary_parser.js";

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

async function request_config() {
    const buffer = await ws.request(PacketType.GET_CONFIG);
    const parser = new BinaryParser(buffer);

    const size = parser.readUInt8();
    if (size < 26) {
        console.error("Invalid config response");
        return {};
    }

    return {
        power: parser.readBoolean(),

        maxBrightness: parser.readUInt8(),

        speed: parser.readUInt8(),
        scale: parser.readUInt8(),
        light: parser.readUInt8(),
        eco: parser.readUInt8(),

        palette: parser.readUInt8(),
        colorEffect: parser.readUInt8(),
        brightnessEffect: parser.readUInt8(),

        colorCorrection: parser.readUInt32(),

        nightMode: {
            enabled: parser.readBoolean(),
            brightness: parser.readUInt8(),
            eco: parser.readUInt8(),

            startTime: parser.readUInt32(),
            endTime: parser.readUInt32(),
            switchInterval: parser.readUInt16(),
        }
    };
}

function createSelect(list, value, cmd) {
    const control = document.createElement("div");
    control.classList.add("input");

    const select = document.createElement("select")
    control.appendChild(select);

    const dOpt = document.createElement("option");
    dOpt.innerText = "...";
    dOpt.setAttribute("hidden", "");
    dOpt.setAttribute("disabled", "");
    select.appendChild(dOpt);

    for (let i = 0; i < list.length; i++) {
        const opt = document.createElement("option");
        opt.value = list[i].code;
        opt.innerText = list[i].name;
        select.appendChild(opt);
    }

    control.__setValue = (v) => {
        select.value = v;
        select.__value = v;
    }

    select.__busy = false;
    control.__setValue(value);

    select.onchange = async () => {
        if (select.__busy) return;

        const savingValue = select.value;

        try {
            select.__busy = true;
            await ws.request(cmd, Uint8Array.of(Number.parseInt(savingValue)).buffer);

            select.value = savingValue;
            select.__value = select.value;
        } catch (e) {
            console.error("Value save error", e);
            select.value = select.__value;
        } finally {
            select.__busy = false;
        }
    }

    control.onclick = (e => {
        const selectRect = select.getBoundingClientRect()

        let direction = 0;
        if (e.clientX < selectRect.left) {
            direction = -1;
        } else if (e.clientX > selectRect.right) {
            direction = 1;
        }

        if (direction === 0) return;

        const value = Number.parseInt(select.value);
        if (!Number.isFinite(value)) return;

        let index = list.findIndex(e => e.code === value);
        if (index >= 0) {
            index = (list.length + index + direction) % list.length;
            select.value = list[index].code.toString();

            select.dispatchEvent(new Event("change"));
        }

        e.preventDefault();
    });

    return control;
}

function createInput(type, value, cmd, size, valueType) {
    const control = document.createElement("input");
    control.classList.add("input");
    control.type = type;
    control.__busy = false;

    control.__setValue = (v) => {
        switch (type) {
            case "time":
                control.valueAsNumber = v * 1000;
                control.onfocus = () => control.showPicker();
                break;

            default:
                control.value = v.toString();
        }
        control.__value = control.value;
    }

    control.__setValue(value);

    control.onchange = async () => {
        if (control.__busy) return;

        let parsedValue;
        switch (type) {
            case "time":
                parsedValue = control.valueAsNumber / 1000;
                break;

            default:
                parsedValue = Number.parseInt(control.value);
        }

        if (Number.isFinite(parsedValue)) {
            const data = new Uint8Array(size);
            const view = new DataView(data.buffer);

            try {
                control.__busy = true;
                control.setAttribute("data-saving", "true");

                view["set" + valueType](0, parsedValue, true);
                await ws.request(cmd, data.buffer);

                control.__value = control.value;
            } catch (e) {
                console.error("Value save error", e);
                control.value = control.__value;
            } finally {
                control.__busy = false;
                control.setAttribute("data-saving", "false");
            }
        }
    }

    return control;
}

function createWheel(value, limit, cmd) {
    const control = document.createElement("div");
    control.classList.add("input");
    control.classList.add("wheel");

    control.__setValue = (v) => {
        control.__value = v;
        control.__pos = (v / limit);

        const percent = (control.__pos * 100);

        if (percent === 0 || percent === 100) {
            control.innerText = percent.toString();
        } else {
            const textValue = percent.toFixed(1).split(".");
            if (textValue[1] === "0") {
                control.innerText = textValue[0];
            } else {
                control.innerHTML = textValue[0] + `<span class='fraction'>.${textValue[1]}</span>`;
            }
        }

        control.style.setProperty("--pos", control.__pos);
    }

    control.__setValue(value);
    control.__busy = false;

    const props = {margin: 20};
    control.onmousedown = control.ontouchstart = (e) => {
        props.active = true;
        props.width = control.getBoundingClientRect().width - props.margin * 2;
        props.left = control.getBoundingClientRect().left + props.margin;

        e.preventDefault();
    }

    control.onmouseup = control.ontouchend = (e) => {
        if (!props.active) return;

        props.active = false;
        e.preventDefault();
    }

    control.onmousemove = control.ontouchmove = async (e) => {
        if (!props.active) return;

        const clientX = e.touches ? e.touches[0].clientX : e.clientX;
        const pos = (clientX - props.left) / props.width;

        const oldValue = control.__value;

        const newPos = Math.max(0, Math.min(1, pos));
        const newValue = Math.round(newPos * limit);

        control.__setValue(newValue);

        if (!control.__busy) {
            control.__busy = true;
            try {
                await ws.request(cmd, Uint8Array.of(control.__value).buffer);
            } catch (e) {
                console.error("Value save error", e);
                control.__setValue(oldValue);
            } finally {
                control.__busy = false;
            }
        }

        e.preventDefault();
    }

    control.onmouseenter = (e) => {
        if (props.active && e.buttons === 0) {
            props.active = false;
        }
    }

    return control;
}

function createTrigger(value, cmdOn, cmdOff = null) {
    const control = document.createElement("a");
    control.classList.add("button");

    control.__setValue = (v) => {
        control.setAttribute("data-value", v.toString());
    }

    control.__setValue(value);

    control.onclick = async () => {
        const value = control.getAttribute("data-value") === "true";
        const nextValue = !value;

        try {
            control.__setValue(nextValue);

            if (cmdOff) {
                await ws.request(nextValue ? cmdOn : cmdOff);
            } else {
                await ws.request(cmdOn, Uint8Array.of(nextValue ? 1 : 0).buffer);
            }
        } catch (e) {
            console.error("Value save error", e);
            control.__setValue(value);
        }
    };

    return control;
}

function startSection(title) {
    const section = document.createElement("div");
    section.classList.add("section");

    const sectionTitle = document.createElement("h3");
    sectionTitle.innerText = title;

    section.appendChild(sectionTitle);
    document.body.appendChild(section);

    return section;
}

function createTitle(title) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    return titleElement;
}

function _getValue(config, prop) {
    let value = prop.key.split(".")
        .reduce((obj, key) => obj[key], config);

    return prop.transform ? prop.transform(value) : value;
}

async function initialize() {
    const config = await request_config();
    console.log("Config", config);

    const [palette, colorEffects, brightnessEffects] = await Promise.all([
        request_fx(PacketType.PALETTE_LIST),
        request_fx(PacketType.COLOR_EFFECT_LIST),
        request_fx(PacketType.BRIGHTNESS_EFFECT_LIST)
    ])

    const Lists = {palette, colorEffects, brightnessEffects};
    const App = {};

    for (const cfg of PropertyConfig) {
        const section = startSection(cfg.section);

        App[cfg.section] = {section, cfg, props: {}};

        for (const prop of cfg.props) {
            const title = createTitle(prop.title);
            section.appendChild(title);

            const value = _getValue(config, prop);

            let control = null;
            switch (prop.type) {
                case "trigger":
                    control = createTrigger(value, ...(Array.isArray(prop.cmd) ? prop.cmd : [prop.cmd]));
                    break;

                case "wheel":
                    control = createWheel(value, prop.limit, prop.cmd);
                    break;

                case "time":
                    control = createInput("time", value, prop.cmd, prop.size, prop.kind);
                    break;

                case "select":
                    control = createSelect(Lists[prop.list], value, prop.cmd);
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
    const config = await request_config();
    console.log("New config", config);

    for (const cfg of PropertyConfig) {
        const section = window.__app.App[cfg.section];
        for (const prop of cfg.props) {
            section.props[prop.key].control.__setValue(_getValue(config, prop));
        }
    }

    window.__app.Config = config;
}

ws.begin();