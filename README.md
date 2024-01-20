# ESP LED LAMP

Beautifull ambient lighting for LED strips and matrices.

## Application UI

<p align="center">
  <img alt="UI" height="480" src="https://github.com/DrA1ex/esp_led_lamp/assets/1194059/2528b31c-21ac-4806-b7c1-755c0d8b66e1">
</p>

## Installation

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
