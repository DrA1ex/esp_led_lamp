"""Build and gzip the WebUI for LittleFS. Can be run or imported by other tools."""

import gzip
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


class BundleError(RuntimeError):
    pass


def build_web_bundle(root=ROOT):
    root = Path(root).resolve()
    web = root / "www"
    destination = root / "data"
    npm = shutil.which("npm.cmd" if os.name == "nt" else "npm")
    if not npm:
        raise BundleError("npm was not found in PATH")

    subprocess.run([npm, "install", "--no-audit", "--no-fund"], cwd=web, check=True)
    binary = web / "node_modules" / ".bin" / ("esbuild.cmd" if os.name == "nt" else "esbuild")
    if not binary.is_file():
        raise BundleError("esbuild was not installed")

    work = root / ".pio"
    work.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="webui-", dir=work) as tmp:
        stage = Path(tmp) / "data"
        stage.mkdir()
        subprocess.run([
            str(binary), "./src/index.js", "./src/service_worker.js",
            "--bundle", "--format=esm", "--minify", "--outdir=" + str(stage)
        ], cwd=web, check=True)

        for filename in ("index.html", "style.css"):
            shutil.copy2(web / "src" / filename, stage / filename)
        for asset in (web / "favicons").iterdir():
            if asset.is_file():
                shutil.copy2(asset, stage / asset.name)

        for file in stage.rglob("*"):
            if not file.is_file():
                continue
            compressed = file.with_name(file.name + ".gz")
            with file.open("rb") as source, compressed.open("wb") as target:
                with gzip.GzipFile(fileobj=target, mode="wb", compresslevel=9, mtime=0) as out:
                    shutil.copyfileobj(source, out)
            file.unlink()

        if (destination / "__storage").exists():
            raise BundleError("data/__storage must not be overwritten by the WebUI build")
        previous = Path(tmp) / "previous"
        if destination.exists():
            destination.rename(previous)
        try:
            stage.rename(destination)
        except OSError:
            if previous.exists():
                previous.rename(destination)
            raise

    return destination


if __name__ == "__main__":
    try:
        print("WebUI bundle:", build_web_bundle())
    except (BundleError, OSError, subprocess.CalledProcessError) as exc:
        sys.exit("WebUI build failed: " + str(exc))
