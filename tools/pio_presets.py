"""PlatformIO pre-script: apply the same preset to builds and both upload types."""

import os
import sys
from pathlib import Path

Import("env")

sys.path.insert(0, str(Path(env.subst("$PROJECT_DIR")) / "tools"))
from presets import PresetError, load_preset, render_header, upload_port


def configure(build_env):
    root = Path(build_env.subst("$PROJECT_DIR"))
    name = os.environ.get("LAMP_PRESET", build_env.GetProjectOption("custom_preset", "")).strip()
    credentials = os.environ.get("LAMP_CREDENTIALS")
    firmware, destination = load_preset(root, name, credentials)

    build_dir = Path(build_env.subst("$BUILD_DIR"))
    # Stable paths let SCons reuse library objects and rebuild header dependants.
    include_dir = build_dir / "config"
    include_dir.mkdir(parents=True, exist_ok=True)
    header = include_dir / "lamp_preset.h"
    content = render_header(firmware)
    if not header.exists() or header.read_text(encoding="utf-8") != content:
        header.write_text(content, encoding="utf-8")
    build_env.Prepend(CPPPATH=[str(include_dir)])

    method = os.environ.get("LAMP_UPLOAD_METHOD")
    if method:
        if method not in ("usb", "ota"):
            raise PresetError("LAMP_UPLOAD_METHOD must be usb or ota.")
        build_env.Replace(UPLOAD_PROTOCOL="espota" if method == "ota" else "esptool")
    else:
        method = "ota" if build_env.subst("$UPLOAD_PROTOCOL") == "espota" else "usb"
    # PlatformIO has already populated UPLOAD_PORT from --upload-port or INI.
    if not build_env.get("UPLOAD_PORT"):
        port = upload_port(firmware, destination, method)
        if port:
            build_env.Replace(UPLOAD_PORT=port)
    if name:
        print("Device preset: " + name)


try:
    configure(env)
except PresetError as error:
    sys.stderr.write("Preset error: " + str(error) + "\n")
    env.Exit(1)
