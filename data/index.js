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
        console.log("Connection lost");
        window.__app.connected = false;
        LoaderElement.style.visibility = "visible";

        setTimeout(initWebSocket, 2000);
    }

    ws.onerror = (e) => {
        console.log("WebSocket error", e);
        ws.close();
    }
}

function request(req) {
    const ws = window.__app.ws;
    ws.send(req);

    return new Promise((resolve) => {
        ws.onmessage = async (e) => {
            ws.onmessage = null;

            /** @type Blob */
            const blob = e.data;
            const buffer = await blob.arrayBuffer();
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

            resolve(result);
        };
    })
}

function createSelect(title, list, cmd) {
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

    control.onchange = () => {
        window.__app.ws.send(Uint8Array.of(0x34, 0xaa, cmd, 0x01, Number.parseInt(control.value)));
    }

    for (let i = 0; i < list.length; i++) {
        const opt = document.createElement("option");
        opt.value = list[i].code;
        opt.innerText = list[i].name;
        control.appendChild(opt);
    }

    document.body.appendChild(control);
}

function createTrigger(title, cmdOn, cmdOff) {
    const titleElement = document.createElement("p");
    titleElement.innerText = title;
    document.body.appendChild(titleElement);

    const control = document.createElement("a");
    control.classList.add("button");
    control.setAttribute("data-value", true.toString());
    control.onclick = () => {
        const value = control.getAttribute("data-value") === "true";

        const cmd = value ? cmdOff : cmdOn;
        window.__app.ws.send(Uint8Array.of(0x34, 0xaa, cmd, 0x00));
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
    createTrigger("Power", PacketType.POWER_ON, PacketType.POWER_OFF);

    const sectionTitle = document.createElement("h3");
    sectionTitle.innerText = "FX";
    document.body.appendChild(sectionTitle);

    const palette = await request(Uint8Array.of(0x34, 0xaa, PacketType.PALETTE_LIST, 0x00));
    createSelect("Palette", palette, PacketType.PALETTE);

    const colorEffects = await request(Uint8Array.of(0x34, 0xaa, PacketType.COLOR_EFFECT_LIST, 0x00));
    createSelect("Color Effect", colorEffects, PacketType.COLOR_EFFECT);

    const brightnessEffects = await request(Uint8Array.of(0x34, 0xaa, PacketType.BRIGHTNESS_EFFECT_LIST, 0x00));
    createSelect("Brightness Effect", brightnessEffects, PacketType.BRIGHTNESS_EFFECT);

    window.__app.config = {
        palette,
        colorEffects,
        brightnessEffects
    };
}

initWebSocket();