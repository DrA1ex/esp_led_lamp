import {PacketType} from "./cmd.js";
import {EventEmitter} from "../misc/event_emitter.js";

import {REQUEST_TIMEOUT, REQUEST_SIGNATURE, CONNECTION_TIMEOUT_DELAY_STEP} from "../constants.js";

export class WebSocketInteraction extends EventEmitter {
    static CONNECTED = "ws_interaction_connected";
    static DISCONNECTED = "ws_interaction_disconnected";
    static ERROR = "ws_interaction_error";
    static MESSAGE = "ws_interaction_message";

    #connected = false;
    #ws = null;

    #connectionTimeout = 0;
    #requestQueue = [];

    gateway;
    requestTimeout;

    get connected() {return this.#connected;}

    constructor(gateway, requestTimeout = REQUEST_TIMEOUT) {
        super();

        this.gateway = gateway;
        this.requestTimeout = requestTimeout;
    }

    begin() {
        if (this.#ws) throw new Error("Already initialized");

        this.connect();
    }

    connect() {
        if (!this.connected) {
            this.#init();
        }
    }

    close() {
        if (this.connected) {
            this.#closeConnection(false);
        }
    }

    /**
     * @param {PacketType} cmd
     * @param {ArrayBuffer} [buffer=null]
     * @returns {Promise<ArrayBuffer|*>}
     */
    async request(cmd, buffer = null) {
        if (!this.#connected) {
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
        console.log("Connecting to", this.gateway);

        this.#connected = false;
        this.#ws = new WebSocket(this.gateway);

        this.#ws.onopen = this.#onOpen.bind(this);
        this.#ws.onclose = this.#closeConnection.bind(this);
        this.#ws.onerror = this.#onError.bind(this);
        this.#ws.onmessage = this.#onMessage.bind(this);
    }

    #onOpen() {
        this.#connected = true;
        this.#connectionTimeout = 0;
        console.log("Connection established");

        this.emitEvent(WebSocketInteraction.CONNECTED);
    }

    #onError(e) {
        this.#connected = false;
        console.error("WebSocket error", e);

        this.emitEvent(WebSocketInteraction.ERROR, e);

        this.#closeConnection();
    }

    async #onMessage(e) {
        if (this.#requestQueue.length === 0 || !this.#connected) {
            console.error("Unexpected message", e);
            return;
        }

        this.emitEvent(WebSocketInteraction.MESSAGE, e);

        const request = this.#requestQueue.shift();

        if (e.data instanceof Blob) {
            const buffer = await e.data.arrayBuffer()
            request.resolve(buffer);
        } else {
            request.resolve(e.data);
        }
    }

    #closeConnection(reconnect = true) {
        console.log("Connection closed");

        this.#connected = false;
        this.#requestQueue = [];

        this.#ws.onopen = null;
        this.#ws.onclose = null;
        this.#ws.onerror = null;
        this.#ws.onmessage = null;

        if ([WebSocket.OPEN, WebSocket.CONNECTING].includes(this.#ws.readyState)) {
            this.#ws.close();
        }

        this.emitEvent(WebSocketInteraction.DISCONNECTED);

        if (reconnect) {
            setTimeout(() => this.#init(), this.#connectionTimeout);
            this.#connectionTimeout += CONNECTION_TIMEOUT_DELAY_STEP;
        }
    }
}