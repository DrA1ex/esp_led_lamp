#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

args=()
while (($#)); do
    if [[ "$1" == "--upload-port" ]]; then
        args+=("--port")
    else
        args+=("$1")
    fi
    shift
done
method=usb
if [[ "${OTA:-0}" == 1 ]]; then method=ota; fi
python3 tools/firmware.py --preset "${LAMP_PRESET:-}" --filesystem --upload "$method" --build-web "${args[@]}"
