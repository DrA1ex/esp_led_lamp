const LoaderElement = document.getElementById("loader");

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

        if (!window.__app.config) {
            await loadConfig();
        }

        LoaderElement.style.visibility = "collapse";
    };

    ws.onclose = () => {
        if (window.__app._request) {
            window.__app._request.reject(new Error("Connection closed"));
            window.__app._request = null;
        }

        console.log("Connection lost");
        window.__app.connected = false;
        LoaderElement.style.visibility = "visible";

        setTimeout(initWebSocket, 2000);
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
    if (buffer) {
        ws.send(Uint8Array.of(...SIGNATURE, cmd, buffer.byteLength, ...new Uint8Array(buffer)));
    } else {
        ws.send(Uint8Array.of(...SIGNATURE, cmd, 0x00));
    }

    return new Promise((resolve, reject) => {
        window.__app._request = {resolve, reject};
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
    if (size < 8) {
        console.error("Invalid config response");
        return {};
    }

    return {
        power: view.getUint8(offset++) === 1,

        maxBrightness: view.getUint8(offset++),

        speed: view.getUint8(offset++),
        scale: view.getUint8(offset++),
        light: view.getUint8(offset++),

        palette: view.getUint8(offset++),
        colorEffect: view.getUint8(offset++),
        brightnessEffect: view.getUint8(offset++),
    }
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

function createTrigger(title, value, cmdOn, cmdOff) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("a");
    control.classList.add("button");
    control.setAttribute("data-value", value.toString());
    control.onclick = async () => {
        const value = control.getAttribute("data-value") === "true";

        const cmd = value ? cmdOff : cmdOn;
        await request(cmd);
        control.setAttribute("data-value", (!value).toString());
    };

    document.body.appendChild(control);
}

const PacketType = {
    SPEED: 0,
    SCALE: 1,
    LIGHT: 2,

    MAX_BRIGHTNESS: 20,

    PALETTE: 100,
    COLOR_EFFECT: 101,
    BRIGHTNESS_EFFECT: 102,

    CONFIGURE_PALETTE: 120,

    PALETTE_LIST: 140,
    COLOR_EFFECT_LIST: 141,
    BRIGHTNESS_EFFECT_LIST: 142,

    GET_CONFIG: 160,

    DISCOVERY: 200,

    POWER_OFF: 220,
    POWER_ON: 221,
    RESTART: 222,
};

async function loadConfig() {
    const config = await request_config();
    console.log("Config", config);

    createTrigger("Power", config.power, PacketType.POWER_ON, PacketType.POWER_OFF);

    const sectionTitle = document.createElement("h3");
    sectionTitle.innerText = "FX";
    document.body.appendChild(sectionTitle);

    const palette = await request_fx(PacketType.PALETTE_LIST);
    createSelect("Palette", palette, config.palette, PacketType.PALETTE);

    const colorEffects = await request_fx(PacketType.COLOR_EFFECT_LIST);
    createSelect("Color Effect", colorEffects, config.colorEffect, PacketType.COLOR_EFFECT);

    const brightnessEffects = await request_fx(PacketType.BRIGHTNESS_EFFECT_LIST);
    createSelect("Brightness Effect", brightnessEffects, config.brightnessEffect, PacketType.BRIGHTNESS_EFFECT);

    window.__app.config = {
        config,
        palette,
        colorEffects,
        brightnessEffects
    };
}

initWebSocket();