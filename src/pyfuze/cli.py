from __future__ import annotations

import os
import shutil
import time
import zipfile
from pathlib import Path
from traceback import print_exc

import click

from . import __version__


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.argument(
    "python_project",
    type=click.Path(exists=True, dir_okay=True, path_type=Path),
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
    help="Include additional file or folder (source[:destination]) (repeatable)",
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
    help="Add environment variables (key=value) (repeatable)",
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
        build_dir = Path("build").resolve()
        python_project = python_project.resolve()
        output_folder = (build_dir / python_project.stem).resolve()
        shutil.rmtree(output_folder, ignore_errors=True)
        output_folder.mkdir(parents=True, exist_ok=True)

        dist_dir = Path("dist").resolve()
        dist_dir.mkdir(parents=True, exist_ok=True)

        # copy the stub launcher
        src_com = (Path(__file__).parent / "pyfuze.com").resolve()
        shutil.copy2(src_com, output_folder / "pyfuze.com")
        click.secho("✓ copied pyfuze.com", fg="green")

        # write .python-version
        if python_version:
            (output_folder / ".python-version").write_text(python_version)
            click.secho(f"✓ wrote .python-version ({python_version})", fg="green")

        # write requirements.txt
        if requirements:
            req_path = Path(requirements).resolve()
            if req_path.is_file():
                shutil.copy2(req_path, output_folder / "requirements.txt")
                if req_path.name != "requirements.txt":
                    click.secho(
                        f"✓ copied {req_path.name} to requirements.txt", fg="green"
                    )
                else:
                    click.secho("✓ copied requirements.txt", fg="green")
            else:
                req_list = [r.strip() for r in requirements.split(",")]
                (output_folder / "requirements.txt").write_text("\n".join(req_list))
                click.secho(
                    f"✓ wrote requirements.txt ({len(req_list)} packages)", fg="green"
                )

        # write pyproject.toml
        if pyproject:
            shutil.copy2(pyproject, output_folder / "pyproject.toml")
            if pyproject.name != "pyproject.toml":
                click.secho(f"✓ copied {pyproject.name} to pyproject.toml", fg="green")
            else:
                click.secho("✓ copied pyproject.toml", fg="green")

        # copy uv.lock file
        if uv_lock:
            shutil.copy2(uv_lock, output_folder / "uv.lock")
            if uv_lock.name != "uv.lock":
                click.secho(f"✓ copied {uv_lock.name} to uv.lock", fg="green")
            else:
                click.secho("✓ copied uv.lock", fg="green")

        # create config.txt file
        if python_project.is_file():
            entry = python_project.name
        win_gui_num = 1 if win_gui else 0
        text = f"""entry={entry}
win_gui={win_gui_num}
uv_install_script_windows={uv_install_script_windows}
uv_install_script_unix={uv_install_script_unix}
"""
        if env:
            for e in env:
                key, value = e.split("=", 1)
                text += f"env_{key}={value}\n"
        (output_folder / "config.txt").write_text(text)
        click.secho("✓ wrote config.txt", fg="green")

        # copy python project files
        src_dir = (output_folder / "src").resolve()
        src_dir.mkdir(parents=True, exist_ok=True)

        if python_project.is_dir():
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
                    dest_path = src_dir / pyfile.relative_to(python_project)
                    dest_path.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(pyfile, dest_path)
        else:
            shutil.copy2(python_project, src_dir / python_project.name)

        click.secho(f"✓ copied {python_project.name} to src folder", fg="green")

        # handle additional includes
        if include:
            for include_item in include:
                if ":" in include_item:
                    source, destination = include_item.split(":", 1)
                else:
                    source = include_item
                    destination = "."

                source_path = Path(source)
                if not source_path.exists():
                    click.secho(
                        f"Warning: Source path {source} does not exist", fg="yellow"
                    )
                    continue

                dest_path = (src_dir / destination / source_path.name).resolve()
                dest_path_rel = dest_path.relative_to(output_folder)
                dest_path.parent.mkdir(parents=True, exist_ok=True)
                if source_path.is_file():
                    shutil.copy2(source_path, dest_path)
                    click.secho(
                        f"✓ copied {source_path.name} to {dest_path_rel}", fg="green"
                    )
                elif source_path.is_dir():
                    shutil.copytree(source_path, dest_path, dirs_exist_ok=True)
                    click.secho(
                        f"✓ copied directory {source_path.name} to {dest_path_rel}",
                        fg="green",
                    )

        # build the zip
        zip_path = dist_dir / f"{python_project.stem}.zip"
        zip_path.unlink(missing_ok=True)

        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for root, _, files in os.walk(output_folder):
                for name in files:
                    file_path = Path(root) / name
                    rel_path = file_path.relative_to(build_dir)
                    if name == "pyfuze.com":
                        info = zipfile.ZipInfo(str(rel_path), time.localtime())
                        info.create_system = 3  # Unix
                        info.external_attr = 0o100755 << 16
                        zf.writestr(info, file_path.read_bytes(), zipfile.ZIP_DEFLATED)
                    else:
                        zf.write(file_path, rel_path)

        click.secho(f"Successfully packaged: {zip_path}", fg="green", bold=True)

    except Exception as exc:
        if os.environ.get("PYFUZE_DEBUG") == "1":
            print_exc()
            raise
        click.secho(f"Error: {exc}", fg="red", bold=True)
        raise SystemExit(1)


if __name__ == "__main__":
    cli()
