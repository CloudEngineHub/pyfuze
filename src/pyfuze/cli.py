from __future__ import annotations

import os
import shutil
import time
import zipfile
from typing import Optional
from pathlib import Path
from traceback import print_exc

import click

from . import __version__


@click.group(
    invoke_without_command=True,
    context_settings={"help_option_names": ["-h", "--help"]},
)
@click.option(
    "--version",
    "-v",
    is_flag=True,
    is_eager=True,
    help="Show version and exit",
)
@click.option(
    "--debug",
    "-d",
    is_flag=True,
    help="Enable debug logging",
)
@click.pass_context
def cli(ctx: click.Context, version: bool, debug: bool) -> None:
    """pyfuze — package Python scripts with dependencies."""
    if version:
        click.echo(f"pyfuze v{__version__}")
        ctx.exit()

    if debug:
        os.environ["PYFUZE_DEBUG"] = "1"

    if ctx.invoked_subcommand is None:
        click.echo(ctx.get_help())
        ctx.exit()


@cli.command()
@click.argument(
    "python_file",
    type=click.Path(exists=True, dir_okay=False, path_type=Path),
)
@click.option(
    "--python",
    "python_version",
    required=True,
    help="Target Python version (e.g. 3.11)",
)
@click.option(
    "--reqs",
    "requirements",
    help="Required packages (comma-separated)",
)
def build(
    python_file: Path,
    python_version: str,
    requirements: Optional[str],
) -> None:
    """Package a Python script into a distributable bundle."""
    try:
        if python_file.suffix != ".py":
            click.secho("Error: the input file is not a .py file", fg="red", bold=True)
            raise click.Abort()

        build_dir = Path("build")
        output_folder = build_dir / python_file.stem
        shutil.rmtree(output_folder, ignore_errors=True)
        output_folder.mkdir(parents=True, exist_ok=True)

        dist_dir = Path("dist")
        dist_dir.mkdir(parents=True, exist_ok=True)

        src_com = Path(__file__).parent / "pyfuze.com"
        shutil.copy2(src_com, output_folder / "pyfuze.com")
        click.secho("✓ copied pyfuze.com", fg="green")

        (output_folder / ".python-version").write_text(python_version)
        click.secho(f"✓ wrote .python-version ({python_version})", fg="green")

        if requirements:
            req_list = [r.strip() for r in requirements.split(",")]
            (output_folder / "requirements.txt").write_text("\n".join(req_list))
            click.secho(
                f"✓ wrote requirements.txt ({len(req_list)} packages)", fg="green"
            )
        else:
            (output_folder / "requirements.txt").touch()
            click.secho("✓ created empty requirements.txt", fg="green")

        shutil.copy2(python_file, output_folder / python_file.name)
        click.secho(f"✓ copied {python_file.name}", fg="green")

        zip_path = dist_dir / f"{python_file.stem}.zip"
        zip_path.unlink(missing_ok=True)

        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for root, _, files in os.walk(build_dir):
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
        raise click.Abort()


def app() -> None:
    """Entry point."""
    try:
        cli()
    except click.ClickException as err:
        raise SystemExit(err.exit_code) from None
    except Exception:
        print_exc()
        raise SystemExit(1)


if __name__ == "__main__":
    app()
