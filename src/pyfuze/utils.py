from __future__ import annotations

import os
import sys
import shutil
import subprocess
from pathlib import Path


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
