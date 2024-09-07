# ESP LED LAMP

Beautifull ambient lighting for LED strips and matrices.

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

## Configuration

Configure credentials by editing [src/credentials.h](src/credentials.h).

| Parameter          | Description                                     | Default         |
|--------------------|-------------------------------------------------|-----------------|
| `WIFI_SSID`        | The SSID of the Wi-Fi network to connect to     | "ESP_LED"       |
| `WIFI_PASSWORD`    | The password for the Wi-Fi network               | "12345678"      |
| `WEBAUTH_USER`     | Username for web authentication                  | "esp_led"       |
| `WEBAUTH_PASSWORD` | Password for web authentication                  | "password"      |
| `MQTT_HOST`        | Hostname or IP address for the MQTT broker      | "example.com"   |
| `MQTT_PORT`        | Port number for the MQTT broker                  | 1234            |
| `MQTT_USER`        | Username for MQTT authentication                 | "esp_user"      |
| `MQTT_PASSWORD`    | Password for MQTT authentication                 | "esp_pass"      |


Customize the application for your specific settings by editing [src/constants.h](src/constants.h).

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

## Firmware Installation

### Prerequisites

Ensure the following are installed:

1. [PlatformIO](https://platformio.org/?utm_source=platformio&utm_medium=piohome)
2. [Node.js](https://nodejs.org)

### Uploading Firmware

For uploading the firmware, execute the following commands:

- For wire USB upload:
```sh
pio run -e release
```

- For OTA upload with default address (`esp_lamp.local`):
```sh
pio run -e ota -t upload
```

- For OTA upload with a custom address:
```sh
pio run -e ota -t upload --upload-port esp_lamp_2.local
```

### Uploading File System

To upload the file system, follow these steps:

1. Make the upload_fs.sh script executable:
```sh
chmod +x ./upload_fs.sh
```

- For automatically building the WebUI and uploading it using OTA with the default address (MacOS, *nix):
```sh
OTA=1 ./upload_fs.sh
```

- For uploading OTA with a custom address:
```sh
OTA=1 ./upload_fs.sh --upload-port esp_lamp_2.local
```

- For Windows users use manual build:

1. Build WebUI:
```sh
cd ./www
npm run build
cd ..
```

2. Upload the file system using OTA:
```sh
pio run -t uploadfs -e ota
```
