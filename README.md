# pyfuze

[![GitHub](https://img.shields.io/badge/GitHub-5c5c5c)](https://github.com/TanixLu/pyfuze)
[![PyPI - Version](https://img.shields.io/pypi/v/pyfuze)](https://pypi.org/project/pyfuze/)

## Description

pyfuze packages your Python project into a standalone Actually Portable Executable ([APE](https://justine.lol/ape.html)).

This project is primarily built on top of [cosmopolitan](https://github.com/jart/cosmopolitan) and [uv](https://github.com/astral-sh/uv).

## Packaging Modes

| Mode                                | Standalone | Cross-Platform            |         Size          |
|-----------------------------------|---------------------------------|--------------------------|------------------------|
| Bundle      (default) | **Yes**                         | No (only runs on packaging platform) | Large                 |
| Online      | No(downloads dependencies at runtime)        | **Yes**                  | **Small**             |

In online mode, all dependencies are downloaded to the `--unzip-path`, which by default points to the system's temporary directory.
This ensures a clean runtime environment without polluting the global system.
Currently, online mode primarily supports running on macOS (both ARM64 and AMD64), Linux (AMD64), and Windows (AMD64).

For more details, refer to:
- https://github.com/jart/cosmopolitan/blob/master/ape/specification.md
- https://docs.astral.sh/uv/reference/policies/platforms/


## Install

```bash
pip install pyfuze
```

## Usage

```text
Usage: pyfuze [OPTIONS] PYTHON_PROJECT

  pyfuze packages your Python project into a standalone Actually Portable
  Executable (APE).

Options:
  --mode TEXT                     Packaging mode (bundle or online)  [default:
                                  bundle]
  --output-name TEXT              Output APE name [default:
                                  <project_name>.com]
  --unzip-path TEXT               APE unzip path [default:
                                  /tmp/<project_name>]
  --python TEXT                   Add .python-version file
  --reqs TEXT                     Add requirements.txt file (input comma-
                                  separated string OR file path)
  --pyproject FILE                Add pyproject.toml file
  --uv-lock FILE                  Add uv.lock file
  --entry TEXT                    Entry python file
  --win-gui                       Hide the console window on Windows
  --include TEXT                  Include additional file or folder
                                  (source[::destination]) (repeatable)
  --exclude TEXT                  Exclude path relative to the project root
                                  (repeatable)
  --env TEXT                      Add environment variables such as
                                  INSTALLER_DOWNLOAD_URL,
                                  UV_PYTHON_INSTALL_MIRROR and
                                  UV_DEFAULT_INDEX (key=value) (repeatable)
  --uv-install-script-windows TEXT
                                  UV installation script URI for Windows
                                  [default: https://astral.sh/uv/install.ps1]
  --uv-install-script-unix TEXT   UV installation script URI for Unix
                                  [default: https://astral.sh/uv/install.sh]
  -d, --debug                     Enable debug logging
  -v, --version                   Show the version and exit.
  -h, --help                      Show this message and exit.
```

## Examples

```bash
pyfuze ./examples/simple.py
```
