"""Build/flash a device preset on Windows, macOS or Linux (no shell required)."""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from presets import PresetError, load_preset, upload_port

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", required=True, help="Name of presets/<name>.ini")
    parser.add_argument("--credentials", help="Shared INI file (default: presets/credentials.ini)")
    parser.add_argument("--upload", choices=("usb", "ota"), help="Build and flash over USB/UART or OTA")
    parser.add_argument("--port", help="Override the preset's serial port or OTA host/IP")
    parser.add_argument("--filesystem", action="store_true", help="Build/upload existing data/ instead of firmware")
    parser.add_argument("-e", "--environment", help="PlatformIO environment (default: release, or ota for OTA upload)")
    args = parser.parse_args()
    credentials = str(Path(args.credentials).resolve()) if args.credentials else None
    try:
        firmware, destination = load_preset(ROOT, args.preset, credentials)
        port = args.port or (upload_port(firmware, destination, args.upload) if args.upload else None)
    except PresetError as error:
        parser.error(str(error))
    if args.port and not args.upload:
        parser.error("--port requires --upload")

    pio = shutil.which("pio") or shutil.which("platformio")
    if not pio:
        parser.error("PlatformIO CLI was not found. Install PlatformIO and add pio to PATH.")
    environment = args.environment or ("ota" if args.upload == "ota" else "release")
    command = [pio, "run", "--project-dir", str(ROOT), "-e", environment]
    if args.filesystem:
        command += ["-t", "uploadfs" if args.upload else "buildfs"]
    elif args.upload:
        command += ["-t", "upload"]
    if port:
        command += ["--upload-port", port]
    process_env = os.environ.copy()
    process_env["LAMP_PRESET"] = args.preset
    # Do not let a previous shell's selection override these CLI arguments.
    process_env.pop("LAMP_CREDENTIALS", None)
    process_env.pop("LAMP_UPLOAD_METHOD", None)
    if credentials:
        process_env["LAMP_CREDENTIALS"] = credentials
    if args.upload:
        process_env["LAMP_UPLOAD_METHOD"] = args.upload
    print("Preset: " + args.preset + "; environment: " + environment, flush=True)
    if args.upload:
        print("Upload: " + args.upload + "; destination: " + (port or "automatic serial port detection"), flush=True)
    return subprocess.call(command, cwd=ROOT, env=process_env)


if __name__ == "__main__":
    sys.exit(main())
