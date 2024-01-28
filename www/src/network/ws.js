import {PacketType} from "./cmd.js";
import {EventEmitter} from "../misc/event_emitter.js";

import {
    REQUEST_TIMEOUT,
    REQUEST_SIGNATURE,
    CONNECTION_TIMEOUT_DELAY_STEP,
    CONNECTION_CLOSE_TIMEOUT,
    CONNECTION_TIMEOUT_MAX_DELAY
} from "../constants.js";

/**
 * @enum {string}
 */
const WebSocketState = {
    uninitialized: "uninitialized",
    disconnected: "disconnected",
    connecting: "connecting",
    reconnecting: "reconnecting",
    connected: "connected"
}

export class WebSocketInteraction extends EventEmitter {
    static CONNECTED = "ws_interaction_connected";
    static DISCONNECTED = "ws_interaction_disconnected";
    static ERROR = "ws_interaction_error";
    static MESSAGE = "ws_interaction_message";

    #id = 0;

    #state = WebSocketState.uninitialized;
    #ws = null;

    #reconnectionTimeout = 0;
    #reconnectionTimerId = null;

    #requestQueue = [];

    gateway;
    requestTimeout;

    get connected() {return this.#state === WebSocketState.connected;}
    get state() {return this.#state;}

    constructor(gateway, requestTimeout = REQUEST_TIMEOUT) {
        super();

        this.gateway = gateway;
        this.requestTimeout = requestTimeout;
    }

    begin() {
        if (this.#state !== WebSocketState.uninitialized) throw new Error("Already initialized");

        this.#state = WebSocketState.disconnected;
        this.connect();
    }

    connect() {
        if (this.#state === WebSocketState.uninitialized) throw new Error("Not initialized. Call begin() first.")

        if ([WebSocketState.disconnected, WebSocketState.reconnecting].includes(this.#state)) {
            this.#init();
        }
    }

    close() {
        if (this.#state === WebSocketState.uninitialized) throw new Error("Not initialized. Call begin() first.")

        if (this.#state !== WebSocketState.disconnected) {
            this.#closeConnection(false);
        }
    }

    /**
     * @param {PacketType} cmd
     * @param {ArrayBuffer} [buffer=null]
     * @returns {Promise<ArrayBuffer|*>}
     */
    async request(cmd, buffer = null) {
        if (!this.connected) {
            throw new Error("Not connected");
        }

        if (buffer) {
            this.#ws.send(Uint8Array.of(...REQUEST_SIGNATURE, cmd, buffer.byteLength, ...new Uint8Array(buffer)));
        } else {
            this.#ws.send(Uint8Array.of(...REQUEST_SIGNATURE, cmd, 0x00));
        }

        return new Promise((resolve, reject) => {
            const timer = setTimeout(() => {
                this.#closeConnection();

                reject(new Error("Request timeout"));
            }, this.requestTimeout);

            function _ok(...args) {
                clearTimeout(timer);
                resolve(...args);
            }

            function _fail(...args) {
                clearTimeout(timer);
                reject(...args);
            }

            this.#requestQueue.push({resolve: _ok, reject: _fail});
        });
    }

    #init() {
        console.log("WebSocket connecting to", this.gateway);

        this.#clearConnectionTimer();

        this.#state = WebSocketState.connecting;
        this.#ws = new WebSocket(this.gateway);
        this.#ws.__id = this.#id++;

        this.#ws.onopen = this.#onOpen.bind(this);
        this.#ws.onclose = this.#closeConnection.bind(this);
        this.#ws.onerror = this.#onError.bind(this);
        this.#ws.onmessage = this.#onMessage.bind(this);
    }

    #onOpen() {
        this.#state = WebSocketState.connected;
        this.#reconnectionTimeout = 0;
        console.log(`#${this.#ws.__id}: WebSocket connection established`);

        this.emitEvent(WebSocketInteraction.CONNECTED);
    }

    #onError(e) {
        console.error(`#${this.#ws.__id}: WebSocket error`, e);

        this.emitEvent(WebSocketInteraction.ERROR, e);
        this.#closeConnection();
    }

    async #onMessage(e) {
        if (this.#requestQueue.length === 0 || !this.connected) {
            console.error(`#${this.#ws.__id}: WebSocket unexpected message`, e);
            return;
        }

        this.emitEvent(WebSocketInteraction.MESSAGE, e);

        const request = this.#requestQueue.shift();

        if (e.data instanceof Blob) {
            const buffer = await e.data.arrayBuffer()
            request.resolve(buffer);
        } else if (typeof e.data === "string") {
            if (e.data === "OK") request.resolve(e.data);
            else request.reject(new Error(`Bad response: ${e.data}`));
        } else {
            request.resolve(e.data);
        }
    }

    #closeConnection(reconnect = true) {
        console.log(`#${this.#ws.__id}: WebSocket connection closed`);

        this.#requestQueue = [];

        this.#ws.onopen = null;
        this.#ws.onclose = null;
        this.#ws.onerror = null;
        this.#ws.onmessage = null;

        if ([WebSocket.OPEN, WebSocket.CONNECTING].includes(this.#ws.readyState)) {
            const ws = this.#ws;
            this.#ws = null;

            const timerId = setInterval(() => {
                if (ws.readyState === WebSocket.CONNECTING) return;

                ws.close();
                clearInterval(timerId);
            }, CONNECTION_CLOSE_TIMEOUT);
        }

        if (reconnect) {
            this.#reconnectionTimerId = setTimeout(() => this.#init(), this.#reconnectionTimeout);
            console.log("WebSocket: try reconnect after", this.#reconnectionTimeout);

            this.#state = WebSocketState.reconnecting;
            this.#reconnectionTimeout = Math.min(CONNECTION_TIMEOUT_MAX_DELAY, this.#reconnectionTimeout + CONNECTION_TIMEOUT_DELAY_STEP);
        } else {
            this.#clearConnectionTimer();
            this.#state = WebSocketState.disconnected;
            this.#reconnectionTimeout = 0;
        }

        this.emitEvent(WebSocketInteraction.DISCONNECTED);
    }

    #clearConnectionTimer() {
        if (this.#state === WebSocketState.reconnecting) {
            clearTimeout(this.#reconnectionTimerId);

            this.#reconnectionTimerId = null;
            this.#state = WebSocketState.disconnected;
        }
    }
}