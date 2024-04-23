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

Customize the application for your specific settings by editing [src/constants.h](src/constants.h).

- `WIFI_MODE` (0 - AP mode, 1 - STA mode)
- `WIFI_SSID` and `WIFI_PASSWORD`
- `TIME_ZONE` (hour offset for correct work of Night Mode)
- `MDNS_NAME` (mDNS for local network, default address [http://esp_lamp.local](http://esp_lamp.local))
- `LED_PIN` and `LED_TYPE` for your LED strip/matrix configuration
- `WIDTH` and `HEIGHT` for your configuration (count of LEDs for vertical and horizontal)
- `MATRIX_VOLTAGE` and `CURRENT_LIMIT` for your specific power supply

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
