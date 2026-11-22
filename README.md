# ESP LED LAMP

Beautifull ambient lighting for addressable LED strips and matrices.

Other: [[RGB LED](https://github.com/DrA1ex/esp_led)] [[RELAY](https://github.com/DrA1ex/esp_relay)]

Инструкция на [русском](/README_RUSSIAN.md).


<p align="center">
  <img alt="Animation" height="256" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/c33b63fe-1197-439c-8b91-698c3b74b915">
</p>


## Application UI

<p align="center">
  <img alt="UI Light" height="480" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/841e800b-41cc-4933-9e5f-deb624b7f1e0">
  <img alt="UI Dark" height="480" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/9d3c6928-69de-4096-9cef-5e063a872605">
</p>

### Instruction for PWA installation (Web as Application)
For Android:
1. Open Chrome.
2. Navigate to [http://esp_lamp.local](http://esp_lamp.local).
3. Tap "Add to Home screen."

For iOS:
1. Open Safari.
2. Navigate to [http://esp_lamp.local](http://esp_lamp.local).
3. Tap the "Share" button.
4. Select "Add to Home Screen."


## Music-reactive lighting from a computer

Use [ESP LED Music Lab](https://github.com/DrA1ex/esp_led_lab) to play music or
use a microphone and send audio-reactive effects to one or more lamps over Wi-Fi.
Audio analysis runs in the browser; the lamp receives only visualization data.

1. Flash the firmware and connect the lamp and computer to the same local network.
2. Open [Music Lab](https://dra1ex.github.io/esp_led_lab/) in Chrome, then select
   **Lamps**, enter the lamp's IP address or `esp_lamp.local`, and connect.
3. Choose a track, the demo beat or **Microphone**, then select an effect in
   **Reaction**. Use **All lamps** or select an individual lamp to customize it.

The connection uses the lamp's WebSocket `/audio` endpoint (normally port 80).
On Chrome, grant local-network access when prompted. Local HTTP/WebSocket devices
may not work from the hosted HTTPS page in other browsers; run Music Lab locally
if the browser blocks the connection.

For message formats and implementation details, see
[Remote Audio protocol](docs/REMOTE_AUDIO_PROTOCOL.md).


## Configuration

Use device presets to keep credentials and hardware settings outside the source files.

1. Copy [presets/credentials.example.ini](presets/credentials.example.ini) to
   `presets/credentials.ini` and enter your shared Wi-Fi, web and MQTT credentials.
2. Copy [presets/device.example.ini](presets/device.example.ini) to, for example,
   `presets/ceiling.ini` and enter that device's settings.
3. Build or flash with `--preset ceiling`; see [upload commands](#uploading-firmware).

Local `presets/*.ini` files are ignored by Git; only `*.example.ini` templates are
versioned. Keep actual passwords out of templates. The old `src/_override/*.h`
files are no longer included.

```ini
[firmware]
WIFI_MODE = WIFI_STA_MODE
MDNS_NAME = "ceiling_lamp"
WIDTH = 153
HEIGHT = 1
LED_COLOR_ORDER = RGB
CURRENT_LIMIT = 2000
MQTT = true
WEB_AUTH = true

[upload]
# Optional; otherwise OTA uses ceiling_lamp.local.
# ota_host = "192.168.1.42"
# Optional; otherwise USB uses automatic serial port detection.
# usb_port = "COM3"
```

INI files use `NAME = value`: double-quoted strings, unquoted numbers, `true` /
`false`, or firmware symbols such as `WIFI_STA_MODE`, `DISABLED`, `WS2812B` and
`RGB`. Quoted strings use JSON-style escaping (`\"` for a quote, `\\` for a
backslash); spaces, `#`, `;` and `%` inside strings are preserved. Put comments on
separate lines beginning with `#` or `;`, not after values. Setting names are case
sensitive; unknown settings stop the build. `WEB_AUTH = false` disables web
authentication.

Settings are applied in this order: defaults in `src/constants.h`,
`src/credentials.h` and `src/sys_constants.h` → shared `presets/credentials.ini`
(if present) → the selected device INI. A device can override any shared setting.
Use `--credentials presets/work.ini` to choose a different shared file.
Firmware keys correspond to the existing `#define` names in those three headers.
Without a preset, PlatformIO uses the defaults plus shared credentials if present.
All presets in the same environment share `.pio/build/<environment>/`. Changing
the generated configuration header rebuilds its dependent sources; unchanged
libraries and Arduino framework objects are reused. The firmware image in this
directory belongs to the last built preset. The wrapper defaults to `release`
for both USB and OTA; `debug` and `local-audio` retain separate build directories.

The following tables describe the defaults, which can be overridden in `[firmware]`.

| Parameter          | Description                                     | Default         |
|--------------------|-------------------------------------------------|-----------------|
| `WIFI_SSID`        | The SSID of the Wi-Fi network to connect to     | "ESP_LED"       |
| `WIFI_PASSWORD`    | The password for the Wi-Fi network               | "12345678"      |
| `AUTH_USER`        | Username for web authentication                  | "esp_lamp"      |
| `AUTH_PASSWORD`    | Password for web authentication                  | "password"      |
| `MQTT_HOST`        | Hostname or IP address for the MQTT broker      | "example.com"   |
| `MQTT_PORT`        | Port number for the MQTT broker                  | 1234            |
| `MQTT_USER`        | Username for MQTT authentication                 | "esp_user"      |
| `MQTT_PASSWORD`    | Password for MQTT authentication                 | "esp_pass"      |


Hardware and application defaults are defined in [src/constants.h](src/constants.h).

| Parameter                             | Description                                                       | Default           |
|---------------------------------------|-------------------------------------------------------------------|-------------------|
| WIFI_MODE                             | Mode of Wi-Fi operation (e.g., AP mode or Station mode)        | `WIFI_AP_MODE`    |
| WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL  | Maximum time (ms) to wait for Wi-Fi connection before switching to AP mode. A value of 0 means it never switches. | `0`               |
| WEB_AUTH                              | Enables web-based authentication features                         | Enabled       |
| TIME_ZONE                             | Time zone offset from GMT (e.g., GMT +5:00)                       | `5.f`             |
| MDNS_NAME                             | Multicast DNS name for the device. (e.g. `esp_lamp`.local)                              | `"esp_lamp"`      |
| BUTTON                                | Indicates whether the button feature is enabled or disabled.     | `DISABLED`        |
| BUTTON_PIN                            | GPIO pin number for the button.                                 | `4u`              |
| LED_PIN                               | GPIO pin number for the LED output.                             | `5u`              |
| LED_TYPE                              | Type of LED used (e.g., WS2812B).                              | `WS2812B`         |
| LED_COLOR_ORDER                       | Order of colors for LED (e.g., RGB, GRB format).                    | `GRB`             |
| CURRENT_LIMIT                         | Maximum current limit for the device in milliamps.              | `500u`            |
| WIDTH                                 | Width dimension for the matrix or display in pixels.           | `16u`             |
| HEIGHT                                | Height dimension for the matrix or display in pixels.          | `16u`             |
| AUDIO                                 | Enables audio features.                                          | `DISABLED`        |
| AUDIO_PIN                             | GPIO pin number assigned for audio output.                      | `0u`              |
| MQTT                                  | Enables MQTT protocol for communication.                        | `0u`              |

## MQTT Topics


| Topic In*            | Topic Out*                | Type           | Values                | Comments                              |
|----------------------|---------------------------|----------------|-----------------------|---------------------------------------|
| MQTT_TOPIC_POWER     | MQTT_OUT_TOPIC_POWER      | uint8_t        | 0..1                  | Power state: ON (1) / OFF (0)         |
| MQTT_TOPIC_BRIGHTNESS| MQTT_OUT_TOPIC_BRIGHTNESS | uint8_t        | 0..255                | Brightness level     				  	|
| MQTT_TOPIC_SPEED     | MQTT_OUT_TOPIC_SPEED      | uint8_t        | 0..255                | Color effect's speed parameter        |
| MQTT_TOPIC_SCALE     | MQTT_OUT_TOPIC_SCALE      | uint8_t        | 0..255                | Color effect's scale parameter        |
| MQTT_TOPIC_LIGHT     | MQTT_OUT_TOPIC_LIGHT      | uint8_t        | 0..255                | Brightness effect's light parameter        |
| MQTT_TOPIC_PRESET    | MQTT_OUT_TOPIC_PRESET     | uint8_t        | 0..23          		    | Preset select					      	|
| MQTT_TOPIC_PALETTE   | MQTT_OUT_TOPIC_PALETTE    | uint8_t        | 0..78        			    | Palette select						|
| MQTT_TOPIC_COLOR     | MQTT_OUT_TOPIC_COLOR      | uint32_t       | RGB (e.g. 0xff00ff )  | Color mode's rgb color                |
| MQTT_TOPIC_NIGHT_MODE| MQTT_OUT_TOPIC_NIGHT_MODE | uint8_t        | 0..1                  | Night mode state: ON (1) / OFF (0)    |

* Actual topic values decalred in constants.h

## Connection

![image](https://github.com/user-attachments/assets/f4c72985-7994-4b61-8701-eda145155122)


## Firmware Installation

### Prerequisites

Ensure the following are installed:

0. Configure app first: [configuration](#configuration)
1. [PlatformIO](https://platformio.org/?utm_source=platformio&utm_medium=piohome)
2. [Node.js](https://nodejs.org)

### Uploading Firmware

Run from the repository root. The Python wrapper works on Windows, macOS and
Linux without Bash or additional Python packages; PlatformIO's `pio` must be on
`PATH`. Use your Python 3 command (`python`, `python3`, or `py -3` on Windows).

```sh
# Build only (does not flash a device).
python tools/firmware.py --preset ceiling

# Build and upload over USB/UART (auto-detect the serial port).
python tools/firmware.py --preset ceiling --upload usb

# Select a serial port explicitly (Windows example).
python tools/firmware.py --preset ceiling --upload usb --port COM3

# Build and upload over Wi-Fi to MDNS_NAME.local or [upload] ota_host.
python tools/firmware.py --preset ceiling --upload ota

# Override the destination for this upload only.
python tools/firmware.py --preset ceiling --upload ota --port 192.168.1.42

# Different shared credentials or build environment.
python tools/firmware.py --preset ceiling --credentials presets/work.ini --environment debug
```

### Updating multiple devices

```sh
python tools/firmware.py --preset all --upload ota --dry-run
python tools/firmware.py --preset all --upload ota
python tools/firmware.py --preset all --upload ota --exclude test
```

`all` selects device presets with `MDNS_NAME` or `ota_host`. Set
`batch = false` in a preset's `[upload]` section to exclude it permanently.
Devices are updated sequentially; failures are reported and the next device
continues. Use `--fail-fast` to stop on errors, or `--verbose` for detailed
output (also saved to `.pio/logs/`).

Use `--preset NAME` for one device and `--port HOST` to override its address.
OTA requires a reachable lamp; the first firmware installation uses USB.

### Updating the WebUI (LittleFS)

Build and gzip the web bundle independently:

```sh
python tools/web_bundle.py
```

Or build it automatically and update one lamp or all device presets:

```sh
python tools/firmware.py --preset ceiling --filesystem --upload ota --build-web
python tools/firmware.py --preset all --filesystem --upload ota --build-web
```

Without `--build-web`, the firmware script uses the existing `data/` bundle.
Each device's settings are backed up and included in its image; an unavailable
device is skipped. Backups are retained in `.pio/storage-backups/`.

For a first filesystem installation (no settings to preserve):

```sh
python tools/firmware.py --preset ceiling --filesystem --upload usb --initialize --build-web
```

`--initialize` is required for USB filesystem uploads and cannot be used
with `--preset all`. The existing `upload_fs.sh` also builds the bundle
automatically (`OTA=1 LAMP_PRESET=ceiling ./upload_fs.sh`).
