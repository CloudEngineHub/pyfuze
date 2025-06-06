# pyfuze

[![GitHub](https://img.shields.io/badge/GitHub-5c5c5c)](https://github.com/TanixLu/pyfuze)
[![PyPI - Version](https://img.shields.io/pypi/v/pyfuze)](https://pypi.org/project/pyfuze/)

## Description

pyfuze packages your Python project into a standalone self-extracting Actually Portable Executable ([APE](https://justine.lol/ape.html)).

## Packaging Modes

| Mode                                | Standalone | Cross-Platform            |         Size          |
|-----------------------------------|---------------------------------|--------------------------|------------------------|
| Bundle Mode (default) | **Yes**                         | No (only runs on packaging platform) | Large                 |
| Portable Mode | No(downloads dependencies at runtime)        | **Yes**                  | **Small**             |

The mode is controlled by parameters like `--include-deps`. Please refer to the explanations in the sections below.


## Install

```bash
pip install pyfuze
```

## Usage

```text
Usage: pyfuze [OPTIONS] PYTHON_PROJECT

  pyfuze packages your Python project into a standalone self-extracting
  Actually Portable Executable (APE).

Options:
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
  --include-uv                    Include uv in the output APE
  --include-python                Include python in the output APE
                                  (automatically includes uv)
  --include-deps                  Include dependencies in the output APE
                                  (automatically includes uv and python)
                                  [default: True]
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
