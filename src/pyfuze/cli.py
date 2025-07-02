from __future__ import annotations

import os
import zipfile
from typing import Any
from pathlib import Path
from traceback import print_exc

import click

from . import __version__
from .utils import *


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
        self.cwd = os.getcwd()
        self.current_env = os.environ.copy()

        self.dest_dir = dest_dir
        self.env = env

    def __enter__(self):
        os.chdir(self.dest_dir)

        os.environ["UV_CACHE_DIR"] = "cache"
        os.environ["UV_UNMANAGED_INSTALL"] = "uv"
        os.environ["VIRTUAL_ENV"] = ".venv"

        for e in self.env:
            key, value = e.split("=", 1)
            os.environ[key] = value

    def __exit__(self, exc_type: Any, exc_value: Any, exc_traceback: Any) -> None:
        os.chdir(self.cwd)
        os.environ.clear()
        os.environ.update(self.current_env)


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


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.argument(
    "python_project",
    type=click.Path(exists=True, dir_okay=True, path_type=Path),
)
@click.option(
    "--mode",
    "mode",
    default="bundle",
    help="""
Available modes:

- bundle(default): Includes Python and all dependencies. Runs only on the same platform. Highest compatibility. Extracts to --unzip-path at runtime.

- online: Small and cross-platform. Extracts and downloads dependencies to --unzip-path at runtime (requires internet).

- portable: Standalone cross-platform executable. No extraction and internet needed. Supports only pure Python and --output-name, --entry, --reqs, --include, --exclude.

\b
""",
)
@click.option(
    "--output-name",
    "output_name",
    help="Output executable name [default: <project_name>.com]",
)
@click.option(
    "--entry",
    "entry",
    default="main.py",
    show_default=True,
    help="Entry Python file. Used when your project is a folder.",
)
@click.option(
    "--reqs",
    "requirements",
    help="Add requirements.txt file to specify dependencies (input comma-separated string OR file path)",
)
@click.option(
    "--include",
    "include",
    multiple=True,
    help="Include extra files or folders (e.g. config.ini) (source[::destination]) (repeatable)",
)
@click.option(
    "--exclude",
    "exclude",
    multiple=True,
    help="Exclude project files or folders (e.g. build.py) (repeatable).",
)
@click.option(
    "--unzip-path",
    "unzip_path",
    help="Unzip path for bundle and online modes (default: /tmp/<project_name>)",
)
@click.option(
    "--python",
    "python_version",
    help="Add .python-version file to specify Python version (e.g. 3.11)",
)
@click.option(
    "--pyproject",
    "pyproject",
    type=click.Path(exists=True, dir_okay=False, path_type=Path),
    help="Include pyproject.toml to specify project dependencies",
)
@click.option(
    "--uv-lock",
    "uv_lock",
    type=click.Path(exists=True, dir_okay=False, path_type=Path),
    help="Include uv.lock file to lock dependencies",
)
@click.option(
    "--win-gui",
    is_flag=True,
    help="Hide the console window on Windows",
)
@click.option(
    "--env",
    "env",
    multiple=True,
    help="Add environment variables such as INSTALLER_DOWNLOAD_URL, UV_PYTHON_INSTALL_MIRROR and UV_DEFAULT_INDEX (key=value) (repeatable)",
)
@click.option(
    "--uv-install-script-windows",
    "uv_install_script_windows",
    default="https://astral.sh/uv/install.ps1",
    show_default=True,
    help="UV installation script URI for Windows (URL or local path)",
)
@click.option(
    "--uv-install-script-unix",
    "uv_install_script_unix",
    default="https://astral.sh/uv/install.sh",
    show_default=True,
    help="UV installation script URI for Unix (URL or local path)",
)
@click.option(
    "--debug",
    "-d",
    is_flag=True,
    help="Enable debug logging",
)
@click.version_option(__version__, "-v", "--version", prog_name="pyfuze")
def cli(
    python_project: Path,
    mode: str,
    output_name: str,
    entry: str,
    requirements: str | None,
    include: tuple[str, ...],
    exclude: tuple[str, ...],
    unzip_path: str,
    python_version: str | None,
    pyproject: Path | None,
    uv_lock: Path | None,
    win_gui: bool,
    env: tuple[str, ...],
    uv_install_script_windows: str,
    uv_install_script_unix: str,
    debug: bool,
) -> None:
    """Package Python projects into executables."""
    try:
        if debug:
            os.environ["PYFUZE_DEBUG"] = "1"

        # validate mode
        possible_modes = ["portable", "bundle", "online"]
        if mode not in possible_modes:
            click.secho(
                f"Invalid mode: {mode}\nPossible modes: {', '.join(possible_modes)}",
                fg="red",
                bold=True,
            )
            raise SystemExit(1)
        click.secho(f"starting packaging in {mode} mode...", fg="green")

        # resolve options
        python_project = python_project.resolve()
        project_name = python_project.stem
        output_name = output_name or f"{project_name}.com"
        unzip_path = unzip_path or f"/tmp/{project_name}"
        entry = python_project.name if python_project.is_file() else entry
        win_gui_num = 1 if win_gui else 0

        # create build and dist directories
        build_dir = Path("build").resolve()
        build_dir.mkdir(parents=True, exist_ok=True)
        dist_dir = Path("dist").resolve()
        dist_dir.mkdir(parents=True, exist_ok=True)

        # exclude build and target directories
        exclude = tuple(list(exclude) + ["build", "dist"])

        # create temp directory
        temp_dir = build_dir / project_name
        clean_folder(temp_dir)

        if mode == "portable":
            # write .args
            (temp_dir / ".args").write_text(f"/zip/src/{entry}")
            click.secho(f"✓ wrote .args", fg="green")
        else:
            # write .python-version
            if python_version:
                (temp_dir / ".python-version").write_text(python_version)
                click.secho(f"✓ wrote .python-version ({python_version})", fg="green")

            # write pyproject.toml
            if pyproject:
                cp(pyproject, temp_dir / "pyproject.toml")
                click.secho(f"✓ wrote pyproject.toml", fg="green")

            # write uv.lock
            if uv_lock:
                cp(uv_lock, temp_dir / "uv.lock")
                click.secho(f"✓ wrote uv.lock", fg="green")

            # write .pyfuze_config.txt
            config_list = [
                f"unzip_path={unzip_path}",
                f"entry={entry}",
                f"win_gui={win_gui_num}",
                f"uv_install_script_windows={uv_install_script_windows}",
                f"uv_install_script_unix={uv_install_script_unix}",
            ]
            for e in env:
                key, value = e.split("=", 1)
                config_list.append(f"env_{key}={value}")
            config_text = "\n".join(config_list)
            (temp_dir / ".pyfuze_config.txt").write_text(config_text)
            click.secho("✓ wrote .pyfuze_config.txt", fg="green")

        # write requirements.txt
        if requirements:
            reqs, req_list = parse_requirements(requirements)
            (temp_dir / "requirements.txt").write_text(reqs)
            click.secho(
                f"✓ wrote requirements.txt ({len(req_list)} packages)", fg="green"
            )

        # copy python project files
        src_dir = temp_dir / "src"
        exclude_path_set = {(python_project / e).resolve() for e in exclude}
        copy_python_source(python_project, src_dir, exclude_path_set)
        click.secho(f"✓ copied {python_project.name} to src folder", fg="green")

        # copy additional includes
        copy_includes(include, src_dir)

        # download dependencies
        if mode == "bundle":
            download_uv_python_deps(
                temp_dir,
                env,
                uv_install_script_windows,
                uv_install_script_unix,
            )
        elif mode == "portable":
            download_portable_deps(
                temp_dir,
                env,
                uv_install_script_windows,
                uv_install_script_unix,
            )

        # copy APE to dist directory
        output_path = dist_dir / output_name
        if mode == "portable":
            copy_ape("python.com", output_path, False)
        else:
            copy_ape("pyfuze.com", output_path, win_gui)

        # add temp directory contents to output APE
        with zipfile.ZipFile(output_path, "a", zipfile.ZIP_DEFLATED) as zf:
            for item in temp_dir.rglob("*"):
                if item.is_file():
                    zf.write(item, str(item.relative_to(temp_dir)))

        click.secho(f"Successfully packaged: {output_path}", fg="green", bold=True)

    except Exception as exc:
        if os.environ.get("PYFUZE_DEBUG") == "1":
            print_exc()
            raise
        click.secho(f"Error: {exc}", fg="red", bold=True)
        raise SystemExit(1)


if __name__ == "__main__":
    cli()
