from __future__ import annotations

import os
import shutil
import zipfile
from pathlib import Path
from traceback import print_exc

import click

from . import __version__
from .utils import set_pe_subsystem_console


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.argument(
    "python_project",
    type=click.Path(exists=True, dir_okay=True, path_type=Path),
)
@click.option(
    "--unzip-path",
    "unzip_path",
    help="Unzip path [default: /tmp/<project_name>]",
)
@click.option(
    "--python",
    "python_version",
    help="Add .python-version file",
)
@click.option(
    "--reqs",
    "requirements",
    help="Add requirements.txt file (input comma-separated string OR requirements.txt path)",
)
@click.option(
    "--pyproject",
    "pyproject",
    type=click.Path(exists=True, dir_okay=False, path_type=Path),
    help="Add pyproject.toml file",
)
@click.option(
    "--uv-lock",
    "uv_lock",
    type=click.Path(exists=True, dir_okay=False, path_type=Path),
    help="Add uv.lock file",
)
@click.option(
    "--entry",
    "entry",
    default="main.py",
    help="Entry python file",
)
@click.option(
    "--win-gui",
    is_flag=True,
    help="Hide the console window on Windows",
)
@click.option(
    "--include",
    "include",
    multiple=True,
    help="Include additional file or folder (source[::destination]) (repeatable)",
)
@click.option(
    "--exclude",
    "exclude",
    multiple=True,
    help="Exclude path relative to the project root (repeatable)",
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
    help="UV installation script URI for Windows",
)
@click.option(
    "--uv-install-script-unix",
    "uv_install_script_unix",
    default="https://astral.sh/uv/install.sh",
    show_default=True,
    help="UV installation script URI for Unix",
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
    unzip_path: str,
    python_version: str | None,
    requirements: str | None,
    pyproject: Path | None,
    uv_lock: Path | None,
    entry: str,
    win_gui: bool,
    include: tuple[str, ...],
    exclude: tuple[str, ...],
    env: tuple[str, ...],
    uv_install_script_windows: str,
    uv_install_script_unix: str,
    debug: bool,
) -> None:
    """pyfuze — package Python scripts with dependencies."""
    if debug:
        os.environ["PYFUZE_DEBUG"] = "1"

    try:
        # create build directory
        build_dir = Path("build").resolve()
        python_project = python_project.resolve()
        project_name = python_project.stem
        if not unzip_path:
            unzip_path = f"/tmp/{project_name}"

        # copy the stub launcher
        src_com = (Path(__file__).parent / "pyfuze.com").resolve()
        dst_com = build_dir / f"{project_name}.com"
        shutil.copy2(src_com, dst_com)
        dst_com.chmod(0o755)
        click.secho(f"✓ copied {dst_com}", fg="green")

        if win_gui:
            click.secho(f"✓ configured as Windows GUI application", fg="green")
        else:
            set_pe_subsystem_console(dst_com)
            click.secho(f"✓ configured as console application", fg="green")

        # add contents to /zip
        with zipfile.ZipFile(dst_com, "a", zipfile.ZIP_DEFLATED) as zf:
            # write .python-version
            if python_version:
                zf.writestr(".python-version", python_version)
                click.secho(f"✓ wrote .python-version ({python_version})", fg="green")

            # write requirements.txt
            if requirements:
                req_path = Path(requirements).resolve()
                if req_path.is_file():
                    reqs = req_path.read_text()
                    req_list = [r.strip() for r in reqs.splitlines() if r.strip()]
                else:
                    req_list = [r.strip() for r in requirements.split(",")]
                    reqs = "\n".join(req_list)
                zf.writestr("requirements.txt", reqs)
                click.secho(
                    f"✓ wrote requirements.txt ({len(req_list)} packages)",
                    fg="green",
                )

            # write pyproject.toml
            if pyproject:
                zf.write(pyproject, "pyproject.toml")
                click.secho("✓ wrote pyproject.toml", fg="green")

            # write uv.lock file
            if uv_lock:
                zf.write(uv_lock, "uv.lock")
                click.secho("✓ wrote uv.lock", fg="green")

            # write .pyfuze_config.txt file
            if python_project.is_file():
                entry = python_project.name
            win_gui_num = 1 if win_gui else 0
            config_text = f"""unzip_path={unzip_path}
entry={entry}
win_gui={win_gui_num}
uv_install_script_windows={uv_install_script_windows}
uv_install_script_unix={uv_install_script_unix}
"""
            if env:
                for e in env:
                    key, value = e.split("=", 1)
                    config_text += f"env_{key}={value}\n"
            zf.writestr(".pyfuze_config.txt", config_text)
            click.secho("✓ wrote .pyfuze_config.txt", fg="green")

            # copy python project files
            if python_project.is_file():
                zf.write(python_project, f"src/{python_project.name}")
            elif python_project.is_dir():
                exclude_path_set = set()
                if exclude:
                    for e in exclude:
                        exclude_path_set.add((python_project / e).resolve())
                for pyfile in python_project.rglob("*.py"):
                    pyfile = pyfile.resolve()
                    if pyfile.is_file() and (
                        pyfile.parent == python_project
                        or (pyfile.parent / "__init__.py").exists()
                    ):
                        if pyfile in exclude_path_set:
                            continue
                        zf.write(pyfile, f"src/{pyfile.relative_to(python_project)}")
            else:
                click.secho(
                    f"Warning: Source path {python_project} does not exist", fg="yellow"
                )

            click.secho(f"✓ copied {python_project.name} to src folder", fg="green")

            # handle additional includes
            if include:
                for include_item in include:
                    if "::" in include_item:
                        source, destination = include_item.rsplit("::", 1)
                    else:
                        source = include_item
                        destination = Path(source).name

                    source_path = Path(source)
                    if not source_path.exists():
                        click.secho(
                            f"Warning: Source path {source} does not exist", fg="yellow"
                        )
                        continue

                    arcname = str(Path("src") / destination)
                    if source_path.is_file():
                        zf.write(source_path, arcname)
                    elif source_path.is_dir():
                        for item in source_path.rglob("*"):
                            if item.is_file():
                                zf.write(
                                    item,
                                    str(Path("src") / item.relative_to(source_path)),
                                )

                    click.secho(f"✓ copied {source_path} to {arcname}", fg="green")

        click.secho(f"Successfully packaged: {dst_com}", fg="green", bold=True)

    except Exception as exc:
        if os.environ.get("PYFUZE_DEBUG") == "1":
            print_exc()
            raise
        click.secho(f"Error: {exc}", fg="red", bold=True)
        raise SystemExit(1)


if __name__ == "__main__":
    cli()
