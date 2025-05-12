from __future__ import annotations

import os
import shutil
import typing as t
import zipfile
import time
from pathlib import Path
from traceback import print_exc

import typer

from . import __version__

# ---------- Typer instance: one global instance ----------------------
_app = typer.Typer(
    add_completion=False,
    rich_markup_mode=None,
    context_settings={"help_option_names": ["-h", "--help"]},
    no_args_is_help=True,
)


# ---------- Callback: global options / version output --------------
@_app.callback(invoke_without_command=True)
def main(
    ctx: typer.Context,
    version: bool = typer.Option(
        False,
        "--version",
        "-v",
        is_flag=True,
        help="Show version and exit",
        is_eager=True,
    ),
    debug: bool = typer.Option(
        False, "--debug", "-d", is_flag=True, help="Enable debug logging"
    ),
):
    """
    [bold cyan]pyfuze[/] — Package Python scripts with dependencies.
    """
    # Handle version flag first (eager)
    if version:
        print(f"[green]pyfuze v{__version__}[/]")
        raise typer.Exit()

    if debug:
        os.environ["PYFUZE_DEBUG"] = "1"

    # If no command was invoked, show help
    if ctx.invoked_subcommand is None:
        print(ctx.get_help())
        raise typer.Exit()


# ---------- Build command -----------------------------------
@_app.command()
def build(
    python_file: str = typer.Argument(..., help="Path to Python file"),
    python_version: str = typer.Option(..., "--python", help="Python version"),
    requirements: t.Optional[str] = typer.Option(
        None, "--reqs", help="Required packages, comma separated"
    ),
):
    """
    Package a Python script into a distributable bundle.
    """
    try:
        # Get input file info
        input_file = Path(python_file)
        if not input_file.exists():
            print(f"[bold red]Error:[/] File {python_file} not found")
            raise typer.Exit(1)

        if not input_file.suffix == ".py":
            print(f"[bold red]Error:[/] {python_file} is not a Python file")
            raise typer.Exit(1)

        # Create output directory structure
        output_folder_name = input_file.stem
        build_dir = Path("build")
        output_folder = build_dir / output_folder_name
        dist_dir = Path("dist")

        # Ensure directories exist
        build_dir.mkdir(exist_ok=True)
        dist_dir.mkdir(exist_ok=True)

        # If output directory already exists, delete it
        if output_folder.exists():
            shutil.rmtree(output_folder)

        # Create output directory
        output_folder.mkdir()

        # Copy pyfuze.com
        pyfuze_com_src = Path(__file__).parent / "pyfuze.com"
        pyfuze_com_dst = output_folder / "pyfuze.com"
        shutil.copy2(pyfuze_com_src, pyfuze_com_dst)
        print("[green]✓[/] Copied pyfuze.com")

        # Create .python-version file
        with open(output_folder / ".python-version", "w") as f:
            f.write(python_version)
        print(f"[green]✓[/] Created .python-version with {python_version}")

        # Create requirements.txt file
        if requirements:
            req_list = [req.strip() for req in requirements.split(",")]
            with open(output_folder / "requirements.txt", "w") as f:
                f.write("\n".join(req_list))
            print(f"[green]✓[/] Created requirements.txt with {len(req_list)} packages")
        else:
            # Create empty requirements.txt file
            (output_folder / "requirements.txt").touch()
            print("[green]✓[/] Created empty requirements.txt")

        # Copy Python script
        shutil.copy2(input_file, output_folder / input_file.name)
        print(f"[green]✓[/] Copied {input_file.name}")

        # Create zip file
        zip_file = dist_dir / f"{output_folder_name}.zip"
        zip_file.unlink(missing_ok=True)
        with zipfile.ZipFile(zip_file, "w", zipfile.ZIP_DEFLATED) as zipf:
            for root, dirs, files in os.walk(build_dir):
                for file in files:
                    file_path = os.path.join(root, file)
                    rel_path = os.path.relpath(file_path, build_dir)
                    if file == "pyfuze.com":
                        info = zipfile.ZipInfo(rel_path, time.localtime())
                        info.create_system = 3  # Unix
                        info.external_attr = 0o100755 << 16
                        zipf.writestr(
                            info, Path(file_path).read_bytes(), zipfile.ZIP_DEFLATED
                        )
                    else:
                        zipf.write(file_path, rel_path)
        print(f"[bold green]Successfully packaged![/] Output file: {zip_file}")

    except Exception as exc:
        if os.environ.get("PYFUZE_DEBUG") == "1":
            print_exc()
            raise
        print(f"[bold red]Error:[/] {str(exc)}")
        raise typer.Exit(1)


# ---------- Entry function --------------------------------------
def app() -> None:
    """Entry point for the application."""
    try:
        _app()
    except Exception as exc:
        print_exc()
        raise SystemExit(1) from exc


if __name__ == "__main__":
    app()
