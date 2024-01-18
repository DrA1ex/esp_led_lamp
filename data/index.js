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
        await request(cmd, Uint8Array.of(Number.parseInt(select.value)).buffer);
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

function createWheel(title, value, limit, cmd) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("div");
    control.classList.add("input");
    control.classList.add("wheel");

    document.body.appendChild(control);

    control.__busy = false;
    control.__value = value;
    control.__pos = (value / limit);
    control.innerText = (control.__pos * 100).toFixed(0);
    control.style.setProperty("--pos", control.__pos);

    // setTimeout(() => {
    //     items.onscroll = async (e) => {
    //         if (control.__busy) return;
    //
    //         const i = Math.round(items.scrollLeft / cellWidth);
    //         if (i < limit) {
    //             const newValue = i;
    //             if (newValue === control.__value) return;
    //
    //             try {
    //                 control.__busy = true;
    //                 await request(cmd, Uint8Array.of(newValue).buffer);
    //                 control.__value = newValue;
    //             } finally {
    //                 control.__busy = false;
    //             }
    //         }
    //     }
    // }, 100);

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
        control.innerText = (control.__pos * 100).toFixed(0);
        control.style.setProperty("--pos", control.__pos.toString());

        if (!control.__busy) {
            control.__busy = true;
            try {
                await request(cmd, Uint8Array.of(control.__value));
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

    createSection("General");
    createTrigger("Power", config.power, PacketType.POWER_ON, PacketType.POWER_OFF);
    createWheel("Brightness", config.maxBrightness, 255, PacketType.MAX_BRIGHTNESS);
    createWheel("ECO", config.eco, 255, PacketType.ECO_LEVEL);

    createSection("FX");

    const palette = await request_fx(PacketType.PALETTE_LIST);
    createSelect("Palette", palette, config.palette, PacketType.PALETTE);

    const colorEffects = await request_fx(PacketType.COLOR_EFFECT_LIST);
    createSelect("Color Effect", colorEffects, config.colorEffect, PacketType.COLOR_EFFECT);

    const brightnessEffects = await request_fx(PacketType.BRIGHTNESS_EFFECT_LIST);
    createSelect("Brightness Effect", brightnessEffects, config.brightnessEffect, PacketType.BRIGHTNESS_EFFECT);

    createSection("Fine Tune");
    createWheel("Speed", config.speed, 255, PacketType.SPEED);
    createWheel("Scale", config.scale, 255, PacketType.SCALE);
    createWheel("Light", config.light, 255, PacketType.LIGHT);

    createSection("Color calibration");
    createWheel("Red", (config.colorCorrection & 0xff0000) >> 16, 255, PacketType.CALIBRATION_R);
    createWheel("Green", (config.colorCorrection & 0xff00) >> 8, 255, PacketType.CALIBRATION_G);
    createWheel("Blue", config.colorCorrection & 0xff, 255, PacketType.CALIBRATION_B);

    createSection("Night Mode");
    createTrigger("Enabled", config.nightMode.enabled, PacketType.NIGHT_MODE_ENABLED);
    createWheel("Brightness", config.nightMode.brightness, 255, PacketType.NIGHT_MODE_BRIGHTNESS);
    createWheel("Eco", config.nightMode.eco, 255, PacketType.NIGHT_MODE_ECO);
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