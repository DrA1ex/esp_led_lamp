#!/bin/bash

OTA=${OTA:0}

set +v

echo "Building..."

cd ./www || (echo "No directory ./www" && exit 1)
npm install || (echo "Unable to install npm packages" && exit 2)
npm run build || (echo "Failed" && exit 3)
cd ..

echo "Compress..."
gzip -9 -r ./data/*

echo "Uploading..."

if (("$OTA" == 1)); then
  echo "*** OTA mode selected ***"
  pio run -t uploadfs -e ota "$@"
else
  echo "*** WIRE mode selected ***"
  pio run -t uploadfs -e release "$@"
fi