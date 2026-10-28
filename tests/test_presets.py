"""Run with: python -m unittest discover -s tests -p test_presets.py"""

import contextlib
import io
import json
import os
import runpy
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import firmware as cli
from presets import PresetError, load_preset, render_header, upload_port


class BuildEnv(dict):
    """The build hook's small SCons interface, without a toolchain or uploader."""

    def __init__(self, root, **values):
        super().__init__(PROJECT_DIR=str(root), BUILD_DIR=str(root / ".pio/build/release"),
                         UPLOAD_PROTOCOL="esptool", **values)
        self.options = {}

    def subst(self, value):
        return self.get(value.lstrip("$"), "")

    def GetProjectOption(self, key, default):
        return self.options.get(key, default)

    def Replace(self, **values):
        self.update(values)

    def Prepend(self, **values):
        self.update(values)


class PresetTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "src").mkdir()
        (self.root / "presets").mkdir()
        for filename in ("constants.h", "credentials.h", "sys_constants.h"):
            shutil.copyfile(ROOT / "src" / filename, self.root / "src" / filename)

    def write(self, name, text):
        target = self.root / "presets" / (name + ".ini")
        target.write_text(text, encoding="utf-8")
        return target

    def hook(self, env, variables=None):
        with patch.dict(os.environ, variables or {}, clear=True), contextlib.redirect_stdout(io.StringIO()):
            runpy.run_path(str(ROOT / "tools/pio_presets.py"),
                           init_globals={"Import": lambda name: None, "env": env})

    def test_shared_credentials_device_override_and_string_escaping(self):
        secret = ' spaces # ; 100% "quote" \\ slash $HOME $(ignored) `ignored` Привет '
        self.write("credentials", '[firmware]\nWIFI_PASSWORD = ' + json.dumps(secret) + '\nWIDTH = 7\n')
        self.write("ceiling", '[firmware]\nWIDTH = 153\nMDNS_NAME = "ceiling"\n')
        values, upload = load_preset(self.root, "ceiling")
        self.assertEqual(json.loads(values["WIFI_PASSWORD"]), secret)
        self.assertEqual(values["WIDTH"], "153")
        self.assertEqual(upload_port(values, upload, "ota"), "ceiling.local")
        self.assertIsNone(upload_port(values, upload, "usb"))

    def test_alternative_credentials_and_optional_default(self):
        self.write("credentials", '[firmware]\nWIFI_SSID = "home"\n')
        work = self.write("work", '[firmware]\nWIFI_SSID = "work"\n')
        self.write("lamp", '[firmware]\nWIDTH = 16\n')
        values, _ = load_preset(self.root, "lamp", work)
        self.assertEqual(values["WIFI_SSID"], '"work"')
        (self.root / "presets/credentials.ini").unlink()
        self.assertEqual(load_preset(self.root), ({}, {}))
        self.assertEqual(upload_port({}, {}, "ota"), "esp_lamp.local")
        with self.assertRaises(PresetError):
            load_preset(self.root, "lamp", "missing.ini")

    def test_upload_overrides_and_legacy_names(self):
        self.write("lamp", '[firmware]\nMDNS_NAME = "esp_lamp_4"\n'
                           '[upload]\nota_host = "192.168.1.42"\nusb_port = "COM3"\n')
        values, upload = load_preset(self.root, "lamp")
        self.assertEqual(upload_port(values, upload, "ota"), "192.168.1.42")
        self.assertEqual(upload_port(values, upload, "usb"), "COM3")
        self.assertEqual(upload_port(values, {}, "ota"), "esp_lamp_4.local")

    def test_auth_can_be_disabled_with_ifdef_and_numeric_values_remain_literals(self):
        self.write("lamp", '[firmware]\nWEB_AUTH = false\nMQTT = true\nWIDTH = 153\n'
                           'TIME_ZONE = -3.5\nLED_COLOR_ORDER = RGB\n')
        values, _ = load_preset(self.root, "lamp")
        header = render_header(values)
        self.assertIn("#undef WEB_AUTH", header)
        self.assertNotIn("#define WEB_AUTH", header)
        self.assertIn("#define MQTT 1", header)
        self.assertIn("#define TIME_ZONE -3.5f", header)
        self.assertIn("#define WIDTH 153", header)

    def test_invalid_configuration_fails_without_echoing_secrets(self):
        invalid = [
            '[firmware]\nWIDHT = 10',
            '[firmware]\nWIFI_PASSWORD = super-secret-without-quotes!',
            '[firmware]\nWIFI_PASSWORD = "super-secret" trailing garbage',
            '[firmware]\nWIDTH = 2\nWIDTH = 3',
            '[firmware]\nWIDTH = 1; injected()',
            '[DEFAULT]\nWIDTH = 5',
            '[unknown]\nWIDTH = 5',
            '[upload]\nport = COM3',
            '[upload]\nota_host = ""',
            '[firmware]\nWEB_AUTH = DISABLED',
            'super-secret-invalid-syntax',
        ]
        for content in invalid:
            with self.subTest(content=content):
                self.write("lamp", content)
                with self.assertRaises(PresetError) as caught:
                    load_preset(self.root, "lamp")
                self.assertNotIn("super-secret", str(caught.exception))
        for name in ("../credentials", "foo/bar", "foo\\bar", "credentials", "bad.name"):
            with self.subTest(name=name), self.assertRaises(PresetError):
                load_preset(self.root, name)

    def test_platformio_hook_selects_profile_and_preserves_explicit_port(self):
        self.write("lamp", '[firmware]\nMDNS_NAME = "lamp"\nWIDTH = 46\n')
        env = BuildEnv(self.root, UPLOAD_PORT="192.168.1.90")
        self.hook(env, {"LAMP_PRESET": "lamp", "LAMP_UPLOAD_METHOD": "ota"})
        self.assertEqual(env["UPLOAD_PORT"], "192.168.1.90")
        self.assertEqual(env["UPLOAD_PROTOCOL"], "espota")
        self.assertEqual(Path(env["BUILD_DIR"]).parts[-2:], ("presets", "lamp"))
        self.assertIn("#define WIDTH 46", (Path(env["CPPPATH"][0]) / "lamp_preset.h").read_text())

    def test_direct_platformio_custom_preset_and_header_refresh(self):
        self.write("lamp", '[firmware]\nMDNS_NAME = "lamp"\nWIDTH = 46\n')
        env = BuildEnv(self.root)
        env.options["custom_preset"] = "lamp"
        env["UPLOAD_PROTOCOL"] = "espota"
        self.hook(env)
        self.assertEqual(env["UPLOAD_PORT"], "lamp.local")
        header = Path(env["CPPPATH"][0]) / "lamp_preset.h"
        initial_mtime = header.stat().st_mtime_ns
        second = BuildEnv(self.root)
        self.hook(second, {"LAMP_PRESET": "lamp"})
        self.assertEqual(header.stat().st_mtime_ns, initial_mtime)
        self.write("lamp", '[firmware]\nWIDTH = 50\n')
        self.hook(BuildEnv(self.root), {"LAMP_PRESET": "lamp"})
        self.assertIn("#define WIDTH 50", header.read_text())
        self.assertNotIn("MDNS_NAME", header.read_text())

    def test_cli_build_and_upload_routing_without_invoking_an_uploader(self):
        self.write("lamp", '[firmware]\nMDNS_NAME = "lamp"\n[upload]\nusb_port = "COM7"\n')
        cases = [
            ([], "release", None, None),
            (["--upload", "usb"], "release", "upload", "COM7"),
            (["--upload", "ota"], "ota", "upload", "lamp.local"),
            (["--upload", "ota", "--port", "192.168.1.42"], "ota", "upload", "192.168.1.42"),
            (["--filesystem", "--upload", "ota"], "ota", "uploadfs", "lamp.local"),
            (["--filesystem"], "release", "buildfs", None),
            (["--upload", "ota", "--environment", "debug"], "debug", "upload", "lamp.local"),
        ]
        for args, environment, target, port in cases:
            with self.subTest(args=args), patch.object(cli, "ROOT", self.root), \
                    patch.object(sys, "argv", ["firmware.py", "--preset", "lamp"] + args), \
                    patch.object(cli.shutil, "which", return_value="pio"), \
                    patch.object(cli.subprocess, "call", return_value=17) as call, \
                    patch.dict(os.environ, {"LAMP_CREDENTIALS": "stale.ini", "LAMP_UPLOAD_METHOD": "usb"}), \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(cli.main(), 17)
                command = call.call_args.args[0]
                self.assertEqual(command[command.index("-e") + 1], environment)
                self.assertEqual(command[command.index("-t") + 1] if "-t" in command else None, target)
                self.assertEqual(command[command.index("--upload-port") + 1] if port else None, port)
                process_env = call.call_args.kwargs["env"]
                self.assertEqual(process_env["LAMP_PRESET"], "lamp")
                self.assertNotIn("LAMP_CREDENTIALS", process_env)
                self.assertNotIn("shell", call.call_args.kwargs)


if __name__ == "__main__":
    unittest.main()
