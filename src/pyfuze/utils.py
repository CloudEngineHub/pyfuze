from __future__ import annotations

import os
import sys
import shutil
import subprocess
from pathlib import Path
from typing import Any
import uuid
from datetime import datetime

import click


def rm(path: str | Path) -> None:
    path = Path(path)
    if path.exists():
        if path.is_dir():
            shutil.rmtree(path)
        else:
            path.unlink()


def cp(src: str | Path, dst: str | Path) -> None:
    src = Path(src)
    dst = Path(dst)
    if dst.exists():
        rm(dst)
    if src.is_dir():
        shutil.copytree(src, dst, dirs_exist_ok=True)
    else:
        if dst.is_dir():
            dst.mkdir(parents=True, exist_ok=True)
        else:
            dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def is_subpath(child: Path, parent: Path) -> bool:
    try:
        child.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def clean_folder(folder_path: str | Path) -> None:
    folder_path = Path(folder_path)
    rm(folder_path)
    folder_path.mkdir(parents=True, exist_ok=True)


def copy_python_source(src: Path, dst: Path, exclude_path_set: set[Path]) -> None:
    if src.is_file():
        cp(src, dst / src.name)
    else:
        for pyfile in src.rglob("*.py"):
            if (
                pyfile.is_file()
                and (pyfile.parent == src or (pyfile.parent / "__init__.py").exists())
                and pyfile not in exclude_path_set
            ):
                cp(pyfile, dst / pyfile.relative_to(src))


def run_cmd(cmd: list[str]) -> None:
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = subprocess.SW_HIDE
        process = subprocess.Popen(
            cmd,
            shell=True,  # NOTE: hide the console window on Windows
            stdin=sys.stdin,
            stdout=sys.stdout,
            stderr=sys.stderr,
            startupinfo=startup,
        )
        process.wait()
    elif os.name == "posix":
        process = subprocess.Popen(
            cmd,
            shell=False,
            stdin=sys.stdin,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        process.wait()
    else:
        raise NotImplementedError(f"Unsupported platform: {os.name}")


def set_pe_subsystem(file_path: str, subsystem_type: int):
    with open(file_path, "rb+") as f:
        # Read e_lfanew to find PE header location
        f.seek(0x3C)
        e_lfanew = int.from_bytes(f.read(4), byteorder="little")

        # Calculate subsystem field offset (PE Header + 92)
        subsystem_offset = e_lfanew + 92

        # Seek to subsystem field and write new value
        f.seek(subsystem_offset)

        # 0x02: GUI, 0x03: Console
        subsystem_bytes = subsystem_type.to_bytes(2, byteorder="little")
        f.write(subsystem_bytes)


# By default, pyfuze.com uses the GUI subsystem
# def set_pe_subsystem_gui(file_path: str):
#     return set_pe_subsystem(file_path, 0x02)


def set_pe_subsystem_console(file_path: str):
    return set_pe_subsystem(file_path, 0x03)


def copy_ape(ape_name: str, out_path: Path, win_gui: bool) -> None:
    ape_path = (Path(__file__).parent / ape_name).resolve()
    cp(ape_path, out_path)

    click.secho(f"✓ copied {out_path}", fg="green")

    if win_gui:
        click.secho(f"✓ configured as Windows GUI application", fg="green")
    else:
        set_pe_subsystem_console(str(out_path))
        click.secho(f"✓ configured as console application", fg="green")

    out_path.chmod(0o755)


def parse_requirements(requirements: str) -> tuple[str, list[str]]:
    req_path = Path(requirements).resolve()
    if req_path.is_file():
        reqs = req_path.read_text()
        req_list = [r.strip() for r in reqs.splitlines() if r.strip()]
    else:
        req_list = [r.strip() for r in requirements.split(",")]
        reqs = "\n".join(req_list)
    return reqs, req_list


def copy_includes(include: tuple[str, ...], dest_dir: Path) -> None:
    for include_item in include:
        if "::" in include_item:
            source, destination = include_item.rsplit("::", 1)
        else:
            source = include_item
            destination = Path(source).name

        source_path = Path(source)
        if not source_path.exists():
            click.secho(f"Warning: Source path {source} does not exist", fg="yellow")
            continue

        dest_path = dest_dir / destination
        cp(source_path, dest_path)

        click.secho(
            f"✓ copied {source_path} to {dest_path.relative_to(dest_dir.parent)}",
            fg="green",
        )


def download_uv(uv_install_script_windows: str, uv_install_script_unix: str) -> None:
    if os.name == "nt":
        if Path(uv_install_script_windows).exists():
            run_cmd(
                [
                    "powershell",
                    "-NoProfile",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    uv_install_script_windows,
                ]
            )
        else:
            run_cmd(
                [
                    "powershell",
                    "-NoProfile",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-c",
                    f"irm {uv_install_script_windows} | iex",
                ]
            )
    elif os.name == "posix":
        if Path(uv_install_script_unix).exists():
            run_cmd(["sh", uv_install_script_unix])
        else:
            run_cmd(["sh", "-c", f"curl -LsSf {uv_install_script_unix} | sh"])
    else:
        raise ValueError(f"Unsupported platform: {os.name}")


def download_python() -> None:
    if os.name == "nt":
        uv_path = ".\\uv\\uv.exe"
    elif os.name == "posix":
        uv_path = "./uv/uv"
    else:
        raise ValueError(f"Unsupported platform: {os.name}")

    run_cmd([uv_path, "python", "install", "--install-dir", "python"])


def find_python_rel_path() -> str:
    for path in Path("python").iterdir():
        if path.is_file() or path.name.startswith("."):
            continue
        return f"python/{path.name}"
    raise ValueError("Python not found")


def get_uv_path() -> str:
    if os.name == "nt":
        return ".\\uv\\uv.exe"
    elif os.name == "posix":
        return "./uv/uv"
    else:
        raise ValueError(f"Unsupported platform: {os.name}")


def download_deps() -> None:
    uv_path = get_uv_path()

    if not Path("pyproject.toml").exists():
        # uv init
        run_cmd([uv_path, "init", "--bare", "--no-workspace"])
    if Path("requirements.txt").exists():
        # uv add
        run_cmd(
            [
                uv_path,
                "add",
                "-r",
                "requirements.txt",
                "--python",
                find_python_rel_path(),
            ]
        )
        rm("requirements.txt")

    # uv sync
    cmd = [uv_path, "sync", "--python", find_python_rel_path()]
    if Path("uv.lock").exists():
        cmd.append("--frozen")
    run_cmd(cmd)

    # rm .venv
    rm(".venv")


class DownloadEnv:
    def __init__(self, dest_dir: Path, env: tuple[str, ...]):
        self.origin_cwd = os.getcwd()
        self.origin_env = os.environ.copy()

        self.dest_dir = dest_dir
        self.env = env

    def __enter__(self):
        os.chdir(self.dest_dir)

        os.environ["UV_CACHE_DIR"] = "cache"
        os.environ["UV_UNMANAGED_INSTALL"] = "uv"
        os.environ["VIRTUAL_ENV"] = ".venv"

        # https://github.com/PowerShell/PowerShell/issues/18530#issuecomment-1325691850
        os.environ["PSModulePath"] = ""

        for e in self.env:
            key, value = e.split("=", 1)
            os.environ[key] = value

    def __exit__(self, exc_type: Any, exc_value: Any, exc_traceback: Any) -> None:
        os.chdir(self.origin_cwd)
        os.environ.clear()
        os.environ.update(self.origin_env)


def download_portable_deps(
    dest_dir: Path,
    env: tuple[str, ...],
    uv_install_script_windows: str,
    uv_install_script_unix: str,
) -> None:
    with DownloadEnv(dest_dir, env):
        if Path("requirements.txt").exists():
            download_uv(uv_install_script_windows, uv_install_script_unix)
            Path(".python-version").write_text("3.12.3")
            download_python()
            rm(".python-version")

            uv_path = get_uv_path()
            site_packages_path = Path("Lib/site-packages")
            site_packages_path.mkdir(parents=True, exist_ok=True)
            run_cmd(
                [
                    uv_path,
                    "pip",
                    "install",
                    "-r",
                    "requirements.txt",
                    "--target",
                    str(site_packages_path),
                    "--python",
                    find_python_rel_path(),
                ]
            )
            rm("requirements.txt")

            rm("uv")
            rm("cache")
            rm("python")

            click.secho(f"✓ downloaded dependencies", fg="green")


def download_uv_python_deps(
    dest_dir: Path,
    env: tuple[str, ...],
    uv_install_script_windows: str,
    uv_install_script_unix: str,
) -> None:
    with DownloadEnv(dest_dir, env):
        download_uv(uv_install_script_windows, uv_install_script_unix)
        click.secho(f"✓ downloaded uv", fg="green")
        download_python()
        click.secho(f"✓ downloaded python", fg="green")
        download_deps()
        click.secho(f"✓ downloaded dependencies", fg="green")


def gen_uuid_with_time() -> str:
    now = datetime.now().astimezone()
    formatted_time = now.strftime("%Y-%m-%d %H:%M:%S.%f")

    offset = now.utcoffset()
    hours_offset = int(offset.total_seconds() / 3600)
    utf_offset_str = f"UTC{hours_offset:+d}"

    return f"{uuid.uuid4()} {formatted_time} {utf_offset_str}"
