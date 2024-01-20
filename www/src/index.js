import {PacketType} from "./network/cmd.js";
import {WebSocketInteraction} from "./network/ws.js";
import {PropertyConfig} from "./config.js";

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
    }
});

ws.subscribe(this, WebSocketInteraction.DISCONNECTED, () => {
    StatusElement.innerText = "NOT CONNECTED";
    StatusElement.style.visibility = "visible";
});

async function request_fx(cmd) {
    const buffer = await ws.request(cmd);
    const view = new DataView(buffer);

    let offset = 0;
    const count = view.getUint8(offset++);

    const result = new Array(count);
    for (let i = 0; i < count; i++) {
        const code = view.getUint8(offset++);
        let nameLength = 0;

        while (view.getUint8(offset + nameLength) !== 0) {
            ++nameLength;
        }

        result[i] = {
            code: code,
            name: new TextDecoder().decode(buffer.slice(offset, offset + nameLength))
        }

        offset += nameLength + 1;
    }

    return result;
}

async function request_config() {
    const buffer = await ws.request(PacketType.GET_CONFIG);
    const view = new DataView(buffer);

    let offset = 0;

    const size = view.getUint8(offset++);
    if (size < 26) {
        console.error("Invalid config response");
        return {};
    }

    const result = {
        power: view.getUint8(offset++) === 1,

        maxBrightness: view.getUint8(offset++),

        speed: view.getUint8(offset++),
        scale: view.getUint8(offset++),
        light: view.getUint8(offset++),
        eco: view.getUint8(offset++),

        palette: view.getUint8(offset++),
        colorEffect: view.getUint8(offset++),
        brightnessEffect: view.getUint8(offset++),
    }

    result.colorCorrection = view.getUint32(offset, true);
    offset += 4;

    result.nightMode = {
        enabled: view.getUint8(offset++) === 1,
        brightness: view.getUint8(offset++),
        eco: view.getUint8(offset++),
    }

    result.nightMode.startTime = view.getUint32(offset, true);
    offset += 4;

    result.nightMode.endTime = view.getUint32(offset, true);
    offset += 4;

    result.nightMode.switchInterval = view.getUint16(offset, true);
    offset += 2;

    return result;
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

    select.value = value;
    select.onchange = async () => {
        await ws.request(cmd, Uint8Array.of(Number.parseInt(select.value)).buffer);
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

    switch (type) {
        case "time":
            control.valueAsNumber = value * 1000;
            control.onfocus = () => control.showPicker();
            break;

        default:
            control.value = value.toString();
    }

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
                await ws.request(cmd, data);
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

    control.__busy = false;
    control.__value = value;
    control.__pos = (value / limit);

    const textValue = (control.__pos * 100).toFixed(1).split(".");
    control.innerHTML = textValue[0] + `<span class='fraction'>.${textValue[1]}</span>`;

    control.style.setProperty("--pos", control.__pos);

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

        control.__pos = Math.max(0, Math.min(1, pos));
        control.__value = Math.round(control.__pos * limit);

        const textValue = (control.__pos * 100).toFixed(1).split(".");
        control.innerHTML = textValue[0] + `<span class='fraction'>.${textValue[1]}</span>`;

        control.style.setProperty("--pos", control.__pos.toString());

        if (!control.__busy) {
            control.__busy = true;
            try {
                await ws.request(cmd, Uint8Array.of(control.__value));
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
    control.setAttribute("data-value", value.toString());
    control.onclick = async () => {
        const value = control.getAttribute("data-value") === "true";
        const nextValue = !value;

        if (cmdOff) {
            await ws.request(nextValue ? cmdOn : cmdOff);
        } else {
            await ws.request(cmdOn, Uint8Array.of(nextValue ? 1 : 0));
        }

        control.setAttribute("data-value", nextValue.toString());
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

async function initialize() {
    const config = await request_config();
    console.log("Config", config);

    const [palette, colorEffects, brightnessEffects] = await Promise.all([
        request_fx(PacketType.PALETTE_LIST),
        request_fx(PacketType.COLOR_EFFECT_LIST),
        request_fx(PacketType.BRIGHTNESS_EFFECT_LIST)
    ])

    const Lists = {palette, colorEffects, brightnessEffects};

    for (const cfg of PropertyConfig) {
        const section = startSection(cfg.section);

        for (const prop of cfg.props) {
            section.appendChild(
                createTitle(prop.title)
            );

            let value = prop.key.split(".")
                .reduce((obj, key) => obj[key], config);

            if (prop.transform) {
                value = prop.transform(value);
            }

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
            }

            if (control) section.appendChild(control);
        }
    }
}

ws.begin();