import os
from pathlib import Path
from subprocess import run

Import("env")

if not env.IsCleanTarget():
    library = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV") / "ESPAsyncWebServer"
    patch = Path(env.subst("$PROJECT_DIR")) / "tools/patches/ESPAsyncWebServer-3.12.1.patch"
    if not library.is_dir():
        raise RuntimeError(f"ESPAsyncWebServer patch: library not found: {library}")
    if not patch.is_file():
        raise RuntimeError(f"ESPAsyncWebServer patch: patch file not found: {patch}")

    library = library.resolve()
    git_env = os.environ.copy()
    # PlatformIO dependencies often lack .git. Prevent git apply from discovering
    # the parent project repository and silently ignoring every patch hunk.
    git_env["GIT_CEILING_DIRECTORIES"] = str(library.parent)
    command = ["git", "-C", str(library), "apply"]

    def git_apply(*options):
        return run(command + list(options) + [str(patch)],
                   capture_output=True, text=True, env=git_env)

    reverse = git_apply("--reverse", "--check")
    forward = git_apply("--check")
    if reverse.returncode == 0 and forward.returncode == 0:
        raise RuntimeError("ESPAsyncWebServer patch: both checks passed; "
                           "git apply did not verify the target files")
    if reverse.returncode != 0:
        if forward.returncode != 0:
            raise RuntimeError("ESPAsyncWebServer patch: incompatible or partially applied: "
                               + forward.stderr.strip())
        result = git_apply()
        if result.returncode:
            raise RuntimeError("ESPAsyncWebServer patch: apply failed: " + result.stderr.strip())
        if (git_apply("--reverse", "--check").returncode != 0 or
                git_apply("--check").returncode == 0):
            raise RuntimeError("ESPAsyncWebServer patch: verification failed")
