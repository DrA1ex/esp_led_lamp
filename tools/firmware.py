"""Build/flash device presets with compact progress on Windows, macOS and Linux."""

import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from presets import PresetError, device_presets, load_preset, upload_port

ROOT = Path(__file__).resolve().parents[1]


@contextmanager
def build_lock(root):
    """Prevent two wrappers from changing/uploading the shared build at once."""
    path = root / ".pio/firmware.lock"
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as stream:
        # msvcrt locks a byte range; keep one byte on all platforms.
        if path.stat().st_size == 0:
            stream.write(b"\0")
            stream.flush()
        stream.seek(0)
        try:
            if os.name == "nt":
                import msvcrt
                msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError:
            raise PresetError("Another firmware command is using the shared build. Wait for it to finish.") from None
        try:
            yield
        finally:
            stream.seek(0)
            if os.name == "nt":
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(stream, fcntl.LOCK_UN)


def run_stage(command, process_env, log, verbose=False, uploading=False):
    """Save every diagnostic, displaying only sparse progress unless requested."""
    log.write("Command: " + repr(command) + "\n")
    log.flush()
    started = last_update = time.monotonic()
    percentage = -25
    errors = []
    process = subprocess.Popen(command, cwd=ROOT, env=process_env, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, encoding="utf-8",
                               errors="replace", bufsize=1)
    try:
        for line in process.stdout:
            log.write(line)
            if verbose:
                print(line, end="", flush=True)
            else:
                clean = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", line).strip()
                if re.search(r"error:|^Error\b|Preset error:|timed out|No Answer", clean, re.IGNORECASE):
                    errors.append(clean[:240])
                    errors = errors[-3:]
                # The incremental check also prints RAM/Flash percentages.
                match = (re.search(r"(\d{1,3})\s*%", clean)
                         if uploading and clean.startswith(("Uploading:", "Writing at ")) else None)
                if match and 0 <= int(match[1]) <= 100:
                    value = int(match[1])
                    if value >= percentage + 25 or (value == 100 and percentage != 100):
                        percentage = value
                        print("    Upload: " + str(value) + "%", flush=True)
                        last_update = time.monotonic()
                elif time.monotonic() - last_update >= 15:
                    print("    Still working (" + str(int(time.monotonic() - started)) + "s)...", flush=True)
                    last_update = time.monotonic()
        code = process.wait()
        if code and not verbose:
            for error in errors:
                print("    " + error, flush=True)
        return code
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        process.stdout.close()
        log.flush()


def make_plan(args, credentials):
    batch = args.preset in ("all", "*")
    if batch and args.upload == "usb":
        raise PresetError("Batch uploads support OTA only. Select one preset for USB/UART.")
    if args.port and (not args.upload or batch):
        raise PresetError("--port requires a single preset with --upload.")
    if args.exclude and not batch:
        raise PresetError("--exclude requires --preset all (or '*').")
    if batch:
        for name in args.exclude:
            load_preset(ROOT, name, credentials)
        names = device_presets(ROOT, credentials, args.exclude)
    else:
        names = [args.preset]
    plan = []
    destinations = {}
    for name in names:
        firmware, destination = load_preset(ROOT, name, credentials)
        port = args.port or (upload_port(firmware, destination, args.upload) if args.upload else None)
        if batch and args.upload:
            key = port.lower().rstrip(".")
            if key in destinations:
                raise PresetError("Presets '" + destinations[key] + "' and '" + name
                                  + "' both target " + port + ". Use --exclude <name> or [upload] batch = false.")
            destinations[key] = name
        plan.append((name, port))
    return plan


def execute(args, credentials, plan, pio, environment):
    run_dir = ROOT / ".pio/logs" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    run_dir.mkdir(parents=True)
    results = []
    for index, (name, port) in enumerate(plan, 1):
        print("[" + str(index) + "/" + str(len(plan)) + "] " + name
              + (" -> " + (port or "automatic serial port") if args.upload else ""), flush=True)
        process_env = os.environ.copy()
        process_env["LAMP_PRESET"] = name
        process_env.pop("LAMP_CREDENTIALS", None)
        process_env.pop("LAMP_UPLOAD_METHOD", None)
        if credentials:
            process_env["LAMP_CREDENTIALS"] = credentials
        if args.upload:
            process_env["LAMP_UPLOAD_METHOD"] = args.upload
        command = [pio, "run", "--project-dir", str(ROOT), "-e", environment]
        if args.verbose:
            command.append("--verbose")
        log_path = run_dir / (name + ".log")
        started = time.monotonic()
        code = 1
        try:
            with log_path.open("w", encoding="utf-8") as log:
                print("    Building " + ("filesystem" if args.filesystem else "firmware") + "...", flush=True)
                build_command = command + (["-t", "buildfs"] if args.filesystem else [])
                code = run_stage(build_command, process_env, log, args.verbose)
                if not code and args.upload:
                    print("    Uploading via " + args.upload.upper() + "...", flush=True)
                    # ESP8266's nobuild skips Arduino setup, including the LD
                    # script needed by upload/uploadfs. Let SCons check the
                    # existing build and reuse its up-to-date firmware objects.
                    upload_command = command + ["-t", "uploadfs" if args.filesystem else "upload"]
                    if port:
                        upload_command += ["--upload-port", port]
                    code = run_stage(upload_command, process_env, log, args.verbose, uploading=True)
        except OSError as error:
            code = 1
            print("    Could not run PlatformIO: " + str(error), flush=True)
        except KeyboardInterrupt:
            print("    INTERRUPTED. Log: " + str(log_path), flush=True)
            return 130
        status = "OK" if code == 0 else "FAILED"
        print("    " + status + " (" + format(time.monotonic() - started, ".1f") + "s)", flush=True)
        if code:
            print("    Log: " + str(log_path), flush=True)
        results.append((name, code))
        if code and args.fail_fast:
            break
    failed = [name for name, code in results if code]
    print("\nResult: " + str(len(results) - len(failed)) + " succeeded, " + str(len(failed))
          + " failed, " + str(len(plan) - len(results)) + " skipped.", flush=True)
    if failed:
        print("Failed: " + ", ".join(failed), flush=True)
    print("Logs: " + str(run_dir), flush=True)
    return 1 if failed else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", required=True, help="Device name, all, or '*' (quote the asterisk)")
    parser.add_argument("--credentials", help="Shared INI file (default: presets/credentials.ini)")
    parser.add_argument("--upload", choices=("usb", "ota"), help="Build and flash over USB/UART or OTA")
    parser.add_argument("--port", help="Override a single preset's serial port or OTA host/IP")
    parser.add_argument("--filesystem", action="store_true", help="Build/upload existing data/ instead of firmware")
    parser.add_argument("-e", "--environment", default="release", help="PlatformIO environment (default: release for USB and OTA)")
    parser.add_argument("--exclude", action="append", default=[], metavar="NAME", help="Skip a preset in all; may be repeated")
    parser.add_argument("--dry-run", action="store_true", help="Show the validated plan without building or uploading")
    parser.add_argument("--fail-fast", action="store_true", help="Stop at the first device failure (default: continue)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show full PlatformIO diagnostics")
    args = parser.parse_args()
    credentials = str(Path(args.credentials).resolve()) if args.credentials else None
    try:
        plan = make_plan(args, credentials)
    except PresetError as error:
        parser.error(str(error))
    print("Environment: " + args.environment + "; devices: " + str(len(plan)), flush=True)
    if args.dry_run:
        for index, (name, port) in enumerate(plan, 1):
            print("[" + str(index) + "/" + str(len(plan)) + "] " + name
                  + (" -> " + (port or "automatic serial port") if args.upload else " (build only)"))
        return 0
    pio = shutil.which("pio") or shutil.which("platformio")
    if not pio:
        parser.error("PlatformIO CLI was not found. Install PlatformIO and add pio to PATH.")
    try:
        with build_lock(ROOT):
            return execute(args, credentials, plan, pio, args.environment)
    except PresetError as error:
        parser.error(str(error))
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main())
