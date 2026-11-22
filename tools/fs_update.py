"""Safe LittleFS snapshots over the lamp's existing HTTP and WebSocket servers."""

import base64
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import socket
import struct
import time
from urllib import error, parse, request

from presets import load_preset

STORAGE_FILES = {
    "config": "STORAGE_CONFIG_VERSION",
    "preset_names": "STORAGE_PRESET_NAMES_VERSION",
    "preset_configs": "STORAGE_PRESET_CONFIG_VERSION",
    "custom_palette": "STORAGE_CUSTOM_PALETTE_VERSION",
    "layout": "STORAGE_LAYOUT_VERSION",
}
STORAGE_HEADER = b"\xcc\xbb\xaa\xff"  # STORAGE_HEADER, little-endian
MAX_FILE_SIZE = 8192
HTTP_TIMEOUT = 2.5
REBOOT_TIMEOUT = 15


class BackupError(RuntimeError):
    pass


class NoRedirect(request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def endpoint(address):
    """Reject URL schemes, paths, credentials and invalid OTA destination names."""
    if not address or any(c.isspace() for c in address):
        raise BackupError("Invalid device address")
    try:
        parsed = parse.urlsplit("http://" + address)
        port = parsed.port if parsed.port is not None else 80
        hostname = parsed.hostname
    except ValueError as exc:
        raise BackupError("Invalid device address") from exc
    if (not hostname or parsed.netloc.endswith(":") or parsed.username or parsed.password or parsed.path
            or parsed.query or parsed.fragment or port <= 0 or port > 65535):
        raise BackupError("Expected a device hostname/IP without a URL path")
    if not re.fullmatch(r"[a-zA-Z0-9_.:\-\[\]]+", address):
        raise BackupError("Invalid characters in device address")
    return hostname, port, parsed.netloc


def auth_header(root, preset, credentials):
    values, _ = load_preset(root, preset, credentials)
    defaults = (Path(root) / "src/credentials.h").read_text(encoding="utf-8")
    login = []
    for key in ("AUTH_USER", "AUTH_PASSWORD"):
        value = values.get(key)
        if value is None:
            match = re.search(r"^#define\s+" + key + r"\s+(\".*\")\s*$", defaults, re.MULTILINE)
            if not match:
                raise BackupError("Cannot resolve firmware web credentials")
            value = match.group(1)
        try:
            login.append(json.loads(value))
        except (ValueError, TypeError) as exc:
            raise BackupError("Invalid firmware web credentials") from exc
    token = base64.b64encode((login[0] + ":" + login[1]).encode("utf-8")).decode("ascii")
    return "Basic " + token


def storage_versions(root):
    content = (Path(root) / "src/sys_constants.h").read_text(encoding="utf-8")
    result = {}
    for filename, constant in STORAGE_FILES.items():
        match = re.search(r"^#define\s+" + constant + r"\s+\(\(uint8_t\)\s*(\d+)\)", content, re.MULTILINE)
        if not match:
            raise BackupError("Missing storage version: " + constant)
        result[filename] = int(match.group(1))
    return result


def _get(host, path, authorization, missing_ok=False):
    _name, _port, netloc = endpoint(host)
    url = "http://" + netloc + path
    headers = {"Authorization": authorization, "Cache-Control": "no-store", "Accept-Encoding": "identity"}
    opener = request.build_opener(request.ProxyHandler({}), NoRedirect())
    try:
        with opener.open(request.Request(url, headers=headers), timeout=HTTP_TIMEOUT) as response:
            if response.status != 200:
                raise BackupError("Unexpected HTTP response from device")
            if response.headers.get("Content-Encoding", "identity").lower() != "identity":
                raise BackupError("Device sent encoded storage instead of raw bytes")
            body = response.read(MAX_FILE_SIZE + 1)
            if len(body) > MAX_FILE_SIZE:
                raise BackupError("Device response exceeds storage size limit")
            return body
    except error.HTTPError as exc:
        if missing_ok and exc.code == 404:
            return None
        raise BackupError("Device returned HTTP " + str(exc.code) + " for " + path) from exc
    except (error.URLError, TimeoutError, OSError) as exc:
        raise BackupError("Cannot reach device at " + host) from exc


def uptime(host, authorization):
    text = _get(host, "/debug", authorization).decode("ascii", errors="replace")
    if not text.startswith("General:\nHeap:"):
        raise BackupError("Device identity probe did not match this firmware")
    match = re.search(r"^Now:\s*(\d+)\s*$", text, re.MULTILINE)
    if not match:
        raise BackupError("Device identity probe has no uptime")
    return int(match.group(1))


def restart(host, authorization):
    """Use existing RESTART command to flush delayed configuration writes."""
    hostname, port, netloc = endpoint(host)
    key = base64.b64encode(secrets.token_bytes(16)).decode("ascii")
    handshake = (
        "GET /ws HTTP/1.1\r\n"
        "Host: " + netloc + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Authorization: " + authorization + "\r\n\r\n"
    )
    try:
        with socket.create_connection((hostname, port), timeout=HTTP_TIMEOUT) as sock:
            sock.settimeout(HTTP_TIMEOUT)
            sock.sendall(handshake.encode("ascii"))
            headers = b""
            while b"\r\n\r\n" not in headers and len(headers) < 4096:
                chunk = sock.recv(1024)
                if not chunk:
                    break
                headers += chunk
            if not headers.startswith(b"HTTP/1.1 101 ") and not headers.startswith(b"HTTP/1.0 101 "):
                raise BackupError("WebSocket restart handshake rejected")
            accept = base64.b64encode(hashlib.sha1(
                (key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode("ascii")
            ).digest())
            if b"sec-websocket-accept: " + accept.lower() not in headers.lower():
                raise BackupError("Invalid WebSocket handshake response")
            payload = bytes((0x34, 0xAA, 222, 0))  # RESTART, existing /ws protocol
            mask = secrets.token_bytes(4)
            sock.sendall(bytes((0x82, 0x80 | len(payload))) + mask
                         + bytes(c ^ mask[i % 4] for i, c in enumerate(payload)))
            # The MCU resets while processing the command; it may not send an ACK.
    except (OSError, TimeoutError) as exc:
        raise BackupError("Could not request restart over WebSocket") from exc


def wait_for_restart(host, authorization, before):
    deadline = time.monotonic() + REBOOT_TIMEOUT
    while time.monotonic() < deadline:
        time.sleep(0.3)
        try:
            after = uptime(host, authorization)
        except BackupError:
            continue
        if after < before:
            return
    raise BackupError("Device restart was not confirmed; refusing to copy possibly stale settings")


def read_storage(host, authorization, versions):
    result = {}
    for filename, version in versions.items():
        content = _get(host, "/__storage/" + filename, authorization, missing_ok=True)
        if content is None:
            continue
        if len(content) < 6 or content[:4] != STORAGE_HEADER:
            raise BackupError("Corrupt or unexpected storage file: " + filename)
        if content[4] != version:
            raise BackupError("Incompatible storage version for " + filename +
                              ": device " + str(content[4]) + ", firmware " + str(version))
        result[filename] = content
    return result


def snapshot(root, preset, host, authorization, backup_root):
    before = uptime(host, authorization)
    restart(host, authorization)
    wait_for_restart(host, authorization, before)
    values = read_storage(host, authorization, storage_versions(root))
    if not values:
        raise BackupError("No saved settings found; use --initialize on a single device")

    destination = Path(backup_root) / preset
    destination.mkdir(mode=0o700, parents=True, exist_ok=False)
    for filename, contents in values.items():
        path = destination / filename
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "wb") as stream:
            stream.write(contents)

    manifest = {
        "preset": preset,
        "host": host,
        "files": {name: hashlib.sha256(data).hexdigest() for name, data in values.items()},
    }
    (destination / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return values, destination


def verify(host, authorization, expected):
    deadline = time.monotonic() + REBOOT_TIMEOUT
    while time.monotonic() < deadline:
        try:
            uptime(host, authorization)
            for filename, content in expected.items():
                actual = _get(host, "/__storage/" + filename, authorization)
                if actual != content:
                    raise BackupError("Stored settings changed after upload: " + filename)
            return
        except BackupError:
            time.sleep(0.5)
    raise BackupError("Unable to verify restored settings after filesystem upload")


def stage_files(source, destination):
    """Copy a previously built WebUI once per device, without any stored settings."""
    import shutil
    source, destination = Path(source), Path(destination)
    if not source.is_dir() or not (source / "index.html").exists() and not (source / "index.html.gz").exists():
        raise BackupError("WebUI data/ is missing; build the web files first")
    if (source / "__storage").exists():
        raise BackupError("Refusing to copy __storage from shared data/: per-device backup required")
    if any(path.is_symlink() for path in source.rglob("*")):
        raise BackupError("WebUI data/ contains symlinks")
    shutil.copytree(source, destination)
