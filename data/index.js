const LoaderElement = document.getElementById("loader");
const StatusElement = document.getElementById("status");

const host = window.location.hostname;
const gateway = `ws://${host !== "localhost" ? host : "esp_lamp.local"}/ws`;
window.__app = {};

function initWebSocket() {
    console.log("Connecting to", gateway);
    const ws = new WebSocket(gateway);

    window.__app.ws = ws;
    window.__app.connected = false;

    ws.onopen = async () => {
        console.log("Connection established");
        window.__app.connected = true;
        window.__app.connectionTimeout = 0;

        if (!window.__app.config) {
            await initialize();
        }

        LoaderElement.style.visibility = "collapse";
        StatusElement.style.visibility = "collapse";
    };

    ws.onclose = () => {
        if (window.__app._request) {
            window.__app._request.reject(new Error("Connection closed"));
            window.__app._request = null;
        }

        console.log("Connection lost");
        window.__app.connected = false;

        StatusElement.style.visibility = "visible";

        setTimeout(initWebSocket, window.__app.connectionTimeout);
        window.__app.connectionTimeout += 1000;
    }

    ws.onerror = (e) => {
        console.log("WebSocket error", e);
        ws.close();
    }

    ws.onmessage = async (e) => {
        if (!window.__app._request) {
            console.error("Unexpected message", e);
            return;
        }

        if (e.data instanceof Blob) {
            const buffer = await e.data.arrayBuffer()
            window.__app._request.resolve(buffer);
        } else {
            window.__app._request.resolve(e.data);
        }

        window.__app._request = null;
    }
}

const SIGNATURE = [0x34, 0xaa];

function request(cmd, buffer = null) {
    const ws = window.__app.ws;
    if (ws.readyState !== WebSocket.OPEN) {
        throw new Error("Not connected");
    }

    if (buffer) {
        ws.send(Uint8Array.of(...SIGNATURE, cmd, buffer.byteLength, ...new Uint8Array(buffer)));
    } else {
        ws.send(Uint8Array.of(...SIGNATURE, cmd, 0x00));
    }

    return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
            window.__app._request = null;
            StatusElement.style.visibility = "visible";
            ws.close();

            reject(new Error("Timeout"));
        }, 1000);

        function _ok(...args) {
            clearTimeout(timer);
            resolve(...args);
        }

        function _fail(...args) {
            clearTimeout(timer);
            reject(...args);
        }

        window.__app._request = {resolve: _ok, reject: _fail};
    })
}

async function request_fx(cmd) {
    const buffer = await request(cmd);
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
    const buffer = await request(PacketType.GET_CONFIG);
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

function createSelect(title, list, value, cmd) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("select");
    control.classList.add("input");

    const dOpt = document.createElement("option");
    dOpt.innerText = "...";
    dOpt.setAttribute("hidden", "");
    dOpt.setAttribute("disabled", "");
    control.appendChild(dOpt);

    for (let i = 0; i < list.length; i++) {
        const opt = document.createElement("option");
        opt.value = list[i].code;
        opt.innerText = list[i].name;
        control.appendChild(opt);
    }

    control.value = value;

    control.onchange = async () => {
        await request(cmd, Uint8Array.of(Number.parseInt(control.value)).buffer);
    }

    document.body.appendChild(control);
}

function createInput(title, value, cmd, size, type) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("input");
    control.classList.add("input");
    control.value = value;
    control.__busy = false;

    control.onchange = async () => {
        if (control.__busy) return;

        const parsed = Number.parseInt(control.value);
        if (Number.isFinite(parsed)) {
            const data = new Uint8Array(size);
            const view = new DataView(data.buffer);

            try {
                control.__busy = true;
                control.setAttribute("data-saving", "true");

                view["set" + type](0, parsed, true);
                await request(cmd, data);
            } finally {
                control.__busy = false;
                control.setAttribute("data-saving", "false");
            }
        }
    }

    document.body.appendChild(control);
}

function createWheel(title, list, value, cmd) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("div");
    control.classList.add("input");
    control.classList.add("wheel");

    const items = document.createElement("div");
    control.appendChild(items);

    const opt1 = document.createElement("div");
    opt1.classList.add("option");
    items.appendChild(opt1);

    for (let i = 0; i < list.length; i++) {
        const opt = document.createElement("div");
        opt.classList.add("option");
        opt.innerText = list[i].name;
        items.appendChild(opt);
    }

    const opt2 = document.createElement("div");
    opt2.classList.add("option");
    items.appendChild(opt2);

    document.body.appendChild(control);

    const cellWidth = opt1.getBoundingClientRect().width
    items.scrollLeft = list.findIndex(v => v.code === value) * cellWidth;
    control.__busy = false;
    control.__value = value;

    setTimeout(() => {
        items.onscroll = async (e) => {
            if (control.__busy) return;

            const i = Math.round(items.scrollLeft / cellWidth);
            if (i < list.length) {
                const newValue = list[i].code;
                if (newValue === control.__value) return;

                try {
                    control.__busy = true;
                    await request(cmd, Uint8Array.of(newValue).buffer);
                    control.__value = newValue;
                } finally {
                    control.__busy = false;
                }
            }
        }
    }, 100);

    const props = {};
    control.onmousedown = (e) => {
        props.active = true;
        props.margin = 20;

        props.width = control.getBoundingClientRect().width - props.margin * 2;
        props.left = control.getBoundingClientRect().left + props.margin;

        e.preventDefault();
    }

    control.onmouseup = (e) => {
        props.active = false;
        e.preventDefault();
    }

    control.onmousemove = (e) => {
        if (!props.active) return;

        const pos = (e.clientX - props.left) / props.width;
        items.scrollLeft = pos * list.length * cellWidth;
    }

    control.onmouseenter = (e) => {
        if (props.active && e.buttons === 0) {
            props.active = false;
        }
    }

    control.onwheel = (e) => {
        if (e.deltaY !== 0 && e.deltaX === 0) {
            items.scrollLeft -= e.wheelDeltaY;
            e.preventDefault();
        }
    }
}

function createTrigger(title, value, cmdOn, cmdOff = null) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("a");
    control.classList.add("button");
    control.setAttribute("data-value", value.toString());
    control.onclick = async () => {
        const value = control.getAttribute("data-value") === "true";
        const nextValue = !value;

        if (cmdOff) {
            await request(nextValue ? cmdOn : cmdOff);
        } else {
            await request(cmdOn, Uint8Array.of(nextValue ? 1 : 0));
        }

        control.setAttribute("data-value", nextValue.toString());
    };

    document.body.appendChild(control);
}

function createSection(title) {
    const sectionTitle = document.createElement("h3");
    sectionTitle.innerText = title;
    document.body.appendChild(sectionTitle);
}

const PacketType = {
    SPEED: 0,
    SCALE: 1,
    LIGHT: 2,

    MAX_BRIGHTNESS: 20,
    ECO_LEVEL: 21,

    NIGHT_MODE_ENABLED: 40,
    NIGHT_MODE_START: 41,
    NIGHT_MODE_END: 42,
    NIGHT_MODE_INTERVAL: 43,
    NIGHT_MODE_BRIGHTNESS: 44,
    NIGHT_MODE_ECO: 45,

    PALETTE: 100,
    COLOR_EFFECT: 101,
    BRIGHTNESS_EFFECT: 102,

    CALIBRATION_R: 120,
    CALIBRATION_G: 121,
    CALIBRATION_B: 122,

    PALETTE_LIST: 140,
    COLOR_EFFECT_LIST: 141,
    BRIGHTNESS_EFFECT_LIST: 142,

    GET_CONFIG: 160,

    DISCOVERY: 200,

    POWER_OFF: 220,
    POWER_ON: 221,
    RESTART: 222,
};

async function initialize() {
    const config = await request_config();
    console.log("Config", config);

    const _256 = Array.from({length: 256}, (_, i) => ({code: i, name: i}));

    createSection("General");
    createTrigger("Power", config.power, PacketType.POWER_ON, PacketType.POWER_OFF);
    createWheel("Brightness", _256, config.maxBrightness, PacketType.MAX_BRIGHTNESS);
    createWheel("ECO", _256, config.eco, PacketType.ECO_LEVEL);

    createSection("FX");

    const palette = await request_fx(PacketType.PALETTE_LIST);
    createSelect("Palette", palette, config.palette, PacketType.PALETTE);

    const colorEffects = await request_fx(PacketType.COLOR_EFFECT_LIST);
    createSelect("Color Effect", colorEffects, config.colorEffect, PacketType.COLOR_EFFECT);

    const brightnessEffects = await request_fx(PacketType.BRIGHTNESS_EFFECT_LIST);
    createSelect("Brightness Effect", brightnessEffects, config.brightnessEffect, PacketType.BRIGHTNESS_EFFECT);

    createSection("Fine Tune");
    createWheel("Speed", _256, config.speed, PacketType.SPEED);
    createWheel("Scale", _256, config.scale, PacketType.SCALE);
    createWheel("Light", _256, config.light, PacketType.LIGHT);

    createSection("Color calibration");
    createWheel("Red", _256, (config.colorCorrection & 0xff0000) >> 16, PacketType.CALIBRATION_R);
    createWheel("Green", _256, (config.colorCorrection & 0xff00) >> 8, PacketType.CALIBRATION_G);
    createWheel("Blue", _256, config.colorCorrection & 0xff, PacketType.CALIBRATION_B);

    createSection("Night Mode");
    createTrigger("Enabled", config.nightMode.enabled, PacketType.NIGHT_MODE_ENABLED);
    createWheel("Brightness", _256, config.nightMode.brightness, PacketType.NIGHT_MODE_BRIGHTNESS);
    createWheel("Eco", _256, config.nightMode.eco, PacketType.NIGHT_MODE_ECO);
    createInput("Start time", config.nightMode.startTime, PacketType.NIGHT_MODE_START, 4, "Uint32");
    createInput("End time", config.nightMode.endTime, PacketType.NIGHT_MODE_END, 4, "Uint32");
    createInput("Switch interval", config.nightMode.switchInterval, PacketType.NIGHT_MODE_INTERVAL, 2, "Uint16");

    window.__app.config = {
        config,
        palette,
        colorEffects,
        brightnessEffects
    };
}

initWebSocket();