from __future__ import annotations

import os
import zipfile
from pathlib import Path
from traceback import print_exc

import click

from . import __version__
from .utils import *


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

        # exclude build and dist directories
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

        # write .build_id.txt
        (temp_dir / ".build_id.txt").write_text(gen_uuid_with_time())
        click.secho(f"✓ wrote .build_id.txt", fg="green")

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
