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
from presets import PresetError, device_presets, load_preset, render_header, upload_port


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
            '[upload]\nbatch = perhaps',
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
        self.assertEqual(Path(env["BUILD_DIR"]), self.root / ".pio/build/release")
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

        self.write("other", '[firmware]\nWIDTH = 7\n')
        other = BuildEnv(self.root)
        self.hook(other, {"LAMP_PRESET": "other"})
        self.assertEqual(other["BUILD_DIR"], env["BUILD_DIR"])
        self.assertEqual(other["CPPPATH"], env["CPPPATH"])
        self.assertIn("#define WIDTH 7", header.read_text())

    def test_cli_build_and_upload_routing_without_invoking_an_uploader(self):
        self.write("lamp", '[firmware]\nMDNS_NAME = "lamp"\n[upload]\nusb_port = "COM7"\n')
        cases = [
            ([], "release", None, None),
            (["--upload", "usb"], "release", "upload", "COM7"),
            (["--upload", "ota"], "release", "upload", "lamp.local"),
            (["--upload", "ota", "--port", "192.168.1.42"], "release", "upload", "192.168.1.42"),
            (["--filesystem"], "release", "buildfs", None),
            (["--upload", "ota", "--environment", "debug"], "debug", "upload", "lamp.local"),
        ]
        for args, environment, target, port in cases:
            with self.subTest(args=args), patch.object(cli, "ROOT", self.root), \
                    patch.object(sys, "argv", ["firmware.py", "--preset", "lamp"] + args), \
                    patch.object(cli.shutil, "which", return_value="pio"), \
                    patch.object(cli, "run_stage", return_value=0) as call, \
                    patch.dict(os.environ, {"LAMP_CREDENTIALS": "stale.ini", "LAMP_UPLOAD_METHOD": "usb"}), \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(cli.main(), 0)
                command = call.call_args.args[0]
                self.assertEqual(command[command.index("-e") + 1], environment)
                targets = [command[i + 1] for i, part in enumerate(command) if part == "-t"]
                self.assertEqual(targets, [target] if target else [])
                self.assertEqual(command[command.index("--upload-port") + 1] if port else None, port)
                process_env = call.call_args.args[1]
                self.assertEqual(process_env["LAMP_PRESET"], "lamp")
                self.assertNotIn("LAMP_CREDENTIALS", process_env)
                if "--upload" in args:
                    self.assertEqual(call.call_count, 2)
                    self.assertNotIn("nobuild", command)
                    build_command = call.call_args_list[0].args[0]
                    self.assertNotIn("--upload-port", build_command)
                    self.assertEqual(build_command, ["pio", "run", "--project-dir", str(self.root),
                                                     "-e", environment]
                                     + (["-t", "buildfs"] if "--filesystem" in args else []))
                    self.assertEqual(process_env["LAMP_UPLOAD_METHOD"], args[args.index("--upload") + 1])
                else:
                    self.assertEqual(call.call_count, 1)
                    self.assertNotIn("LAMP_UPLOAD_METHOD", process_env)

    def test_filesystem_upload_preserves_settings_over_ota_and_requires_initialize_for_usb(self):
        self.write("lamp", '[firmware]\\nMDNS_NAME = "lamp"\\n[upload]\\nusb_port = "COM7"\\n')
        (self.root / "data").mkdir()
        (self.root / "data" / "index.html").write_text("Web UI", encoding="utf-8")
        expected = {"settings.json": b'{"brightness":50}'}

        for options, target, must_backup in (
            (["--filesystem", "--upload", "ota"], "lamp.local", True),
            (["--filesystem", "--upload", "usb", "--initialize"], "COM7", False),
        ):
            commands = []

            def stage(command, env, log, verbose=False, uploading=False):
                commands.append((list(command), dict(env)))
                staged = Path(env["PLATFORMIO_DATA_DIR"])
                self.assertEqual((staged / "index.html").read_text(), "Web UI")
                if must_backup:
                    self.assertEqual((staged / "__storage" / "settings.json").read_bytes(),
                                     expected["settings.json"])
                else:
                    self.assertFalse((staged / "__storage").exists())
                if "buildfs" in command:
                    image = Path(env["PLATFORMIO_BUILD_DIR"]) / "release" / "littlefs.bin"
                    image.parent.mkdir(parents=True, exist_ok=True)
                    image.write_bytes(b"filesystem image")
                return 0

            with self.subTest(options=options), patch.object(cli, "ROOT", self.root), \\
                    patch.object(sys, "argv", ["firmware.py", "--preset", "lamp"] + options), \\
                    patch.object(cli.shutil, "which", return_value="pio"), \\
                    patch.object(cli, "run_stage", side_effect=stage), \\
                    patch.object(cli, "auth_header", return_value="Basic fake"), \\
                    patch.object(cli, "snapshot", return_value=(expected, self.root / "backup.zip")) as backup, \\
                    patch.object(cli, "verify") as verify, \\
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(cli.main(), 0)
                self.assertEqual(len(commands), 2)
                self.assertIn("buildfs", commands[0][0])
                self.assertIn("uploadfs", commands[1][0])
                self.assertEqual(commands[1][0][-2:], ["--upload-port", target])
                self.assertEqual(commands[0][1]["LAMP_UPLOAD_METHOD"],
                                 "ota" if must_backup else "usb")
                if must_backup:
                    backup.assert_called_once()
                    verify.assert_called_once_with(target, "Basic fake", expected)
                else:
                    backup.assert_not_called()
                    verify.assert_not_called()

        with patch.object(cli, "ROOT", self.root), \\
                patch.object(sys, "argv", ["firmware.py", "--preset", "lamp",
                                            "--filesystem", "--upload", "usb"]), \\
                contextlib.redirect_stderr(io.StringIO()), \\
                self.assertRaises(SystemExit) as caught:
            cli.main()
        self.assertEqual(caught.exception.code, 2)

    def test_filesystem_backup_failure_prevents_upload(self):
        from fs_update import BackupError
        self.write("lamp", '[firmware]\\nMDNS_NAME = "lamp"\\n')
        (self.root / "data").mkdir()
        (self.root / "data" / "index.html").write_text("Web UI")
        with patch.object(cli, "ROOT", self.root), \\
                patch.object(sys, "argv", ["firmware.py", "--preset", "lamp",
                                            "--filesystem", "--upload", "ota"]), \\
                patch.object(cli.shutil, "which", return_value="pio"), \\
                patch.object(cli, "auth_header", return_value="Basic fake"), \\
                patch.object(cli, "snapshot", side_effect=BackupError("device offline")), \\
                patch.object(cli, "run_stage") as stage, \\
                contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(cli.main(), 1)
        stage.assert_not_called()
        self.assertIn("Filesystem update stopped", output.getvalue())

    def run_cli(self, args):
        with patch.object(cli, "ROOT", self.root), \
                patch.object(sys, "argv", ["firmware.py"] + args), \
                contextlib.redirect_stdout(io.StringIO()) as output:
            code = cli.main()
        return code, output.getvalue()

    def test_batch_discovers_only_devices_and_honors_opt_out_and_exclusions(self):
        self.write("zebra", '[firmware]\nMDNS_NAME = "zebra"\n')
        self.write("alpha", '[firmware]\nMDNS_NAME = "alpha"\n')
        self.write("device.example", '[firmware]\nMDNS_NAME = "template"\n')
        self.write("work", '[firmware]\nWIFI_SSID = "work"\n')
        self.write("credentials", '[firmware]\nWIFI_SSID = "home"\n')
        self.write("disabled", '[firmware]\nMDNS_NAME = "disabled"\n[upload]\nbatch = false\n')
        self.assertEqual(device_presets(self.root), ["alpha", "zebra"])
        self.assertEqual(device_presets(self.root, exclude=["alpha"]), ["zebra"])
        values, _ = load_preset(self.root, "disabled")
        self.assertEqual(values["MDNS_NAME"], '"disabled"')
        for selector in ("all", "*"):
            with patch.object(cli.shutil, "which", side_effect=AssertionError("dry run must not need pio")):
                code, output = self.run_cli(["--preset", selector, "--upload", "ota", "--dry-run"])
            self.assertEqual(code, 0)
            self.assertIn("[1/2] alpha -> alpha.local", output)
            self.assertIn("[2/2] zebra -> zebra.local", output)
        self.assertFalse((self.root / ".pio").exists())

        # A chosen shared file is never mistaken for a device, even with mDNS defaults.
        shared = self.write("office", '[firmware]\nMDNS_NAME = "office_default"\n')
        self.assertNotIn("office", device_presets(self.root, shared))

    def test_batch_preflight_rejects_duplicate_targets_before_any_build(self):
        self.write("one", '[firmware]\nMDNS_NAME = "same"\n')
        self.write("two", '[firmware]\nMDNS_NAME = "SAME"\n')
        with patch.object(cli, "run_stage") as stage, contextlib.redirect_stderr(io.StringIO()) as error:
            with self.assertRaises(SystemExit) as caught:
                self.run_cli(["--preset", "all", "--upload", "ota"])
        self.assertEqual(caught.exception.code, 2)
        self.assertIn("both target", error.getvalue())
        stage.assert_not_called()
        code, output = self.run_cli(["--preset", "all", "--upload", "ota", "--exclude", "two", "--dry-run"])
        self.assertEqual(code, 0)
        self.assertIn("[1/1] one", output)

    def test_batch_is_sequential_continues_failures_and_never_uploads_failed_build(self):
        for name in ("alpha", "beta", "gamma"):
            self.write(name, '[firmware]\nMDNS_NAME = "' + name + '"\n')
        calls = []

        def stage(command, process_env, log, verbose=False, uploading=False):
            name = process_env["LAMP_PRESET"]
            calls.append((name, uploading))
            # alpha upload fails, beta build fails, gamma still succeeds.
            return int((name == "alpha" and uploading) or name == "beta")

        with patch.object(cli, "run_stage", side_effect=stage), \
                patch.object(cli.shutil, "which", return_value="pio"):
            code, output = self.run_cli(["--preset", "all", "--upload", "ota"])
        self.assertEqual(code, 1)
        self.assertEqual(calls, [("alpha", False), ("alpha", True), ("beta", False), ("gamma", False), ("gamma", True)])
        self.assertIn("1 succeeded, 2 failed, 0 skipped", output)
        self.assertIn("Failed: alpha, beta", output)
        self.assertEqual(len(list((self.root / ".pio/logs").glob("*/*.log"))), 3)
        calls.clear()
        with patch.object(cli, "run_stage", side_effect=stage), \
                patch.object(cli.shutil, "which", return_value="pio"):
            code, output = self.run_cli(["--preset", "all", "--upload", "ota", "--fail-fast"])
        self.assertEqual(calls, [("alpha", False), ("alpha", True)])
        self.assertIn("2 skipped", output)

    def test_invalid_batch_options_and_missing_devices_do_not_start_pio(self):
        self.write("lamp", '[firmware]\nMDNS_NAME = "lamp"\n')
        cases = [
            ["--preset", "all", "--upload", "usb"],
            ["--preset", "all", "--upload", "ota", "--port", "192.168.1.2"],
            ["--preset", "all", "--exclude", "typo"],
            ["--preset", "all", "--exclude", "lamp"],
            ["--preset", "lamp", "--exclude", "lamp"],
            ["--preset", "lamp", "--port", "COM1"],
        ]
        for args in cases:
            with self.subTest(args=args), patch.object(cli, "run_stage") as stage, \
                    contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                self.run_cli(args)
            stage.assert_not_called()

    def test_compact_logging_retains_warnings_and_verbose_shows_them(self):
        command = [sys.executable, "-c", 'print("warning: noisy"); print("RAM: 74.1%"); '
                   'print("Flash: 50.8%"); print("Uploading: 25%"); '
                   'print("Writing at 0x00000000... (50 %)"); print("Uploading: 100%")']
        log = io.StringIO()
        with patch.object(cli, "ROOT", self.root), contextlib.redirect_stdout(io.StringIO()) as output:
            code = cli.run_stage(command, os.environ.copy(), log, uploading=True)
        self.assertEqual(code, 0)
        self.assertNotIn("warning:", output.getvalue())
        self.assertEqual([line.strip() for line in output.getvalue().splitlines()],
                         ["Upload: 25%", "Upload: 50%", "Upload: 100%"])
        self.assertIn("Upload: 100%", output.getvalue())
        self.assertIn("warning: noisy", log.getvalue())
        with patch.object(cli, "ROOT", self.root), contextlib.redirect_stdout(io.StringIO()) as output:
            cli.run_stage(command, os.environ.copy(), io.StringIO(), verbose=True)
        self.assertIn("warning: noisy", output.getvalue())

        failing = [sys.executable, "-c", 'print("warning: noisy"); print("Error: unavailable"); raise SystemExit(7)']
        log = io.StringIO()
        with patch.object(cli, "ROOT", self.root), contextlib.redirect_stdout(io.StringIO()) as output:
            code = cli.run_stage(failing, os.environ.copy(), log)
        self.assertEqual(code, 7)
        self.assertIn("Error: unavailable", output.getvalue())
        self.assertNotIn("warning: noisy", output.getvalue())
        self.assertIn("warning: noisy", log.getvalue())

    def test_interruption_terminates_child_and_does_not_advance_batch(self):
        process = unittest.mock.MagicMock()
        process.stdout.__iter__.side_effect = KeyboardInterrupt
        process.poll.return_value = None
        with patch.object(cli.subprocess, "Popen", return_value=process), self.assertRaises(KeyboardInterrupt):
            cli.run_stage(["pio"], {}, io.StringIO())
        process.terminate.assert_called_once()
        process.stdout.close.assert_called_once()
        for name in ("alpha", "beta"):
            self.write(name, '[firmware]\nMDNS_NAME = "' + name + '"\n')
        with patch.object(cli, "run_stage", side_effect=KeyboardInterrupt) as stage, \
                patch.object(cli.shutil, "which", return_value="pio"):
            code, output = self.run_cli(["--preset", "all", "--upload", "ota"])
        self.assertEqual(code, 130)
        self.assertEqual(stage.call_count, 1)
        self.assertIn("INTERRUPTED", output)

    def test_build_lock_rejects_overlap_and_releases_on_error(self):
        with self.assertRaises(RuntimeError):
            with cli.build_lock(self.root):
                with self.assertRaises(PresetError):
                    with cli.build_lock(self.root):
                        self.fail("overlapping shared builds must be rejected")
                raise RuntimeError("test")
        with cli.build_lock(self.root):
            pass


if __name__ == "__main__":
    unittest.main()
