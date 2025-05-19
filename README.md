# pyfuze

[![GitHub](https://img.shields.io/badge/GitHub-5c5c5c)](https://github.com/TanixLu/pyfuze)
[![PyPI - Version](https://img.shields.io/pypi/v/pyfuze)](https://pypi.org/project/pyfuze/)

## description

pyfuze makes your Python project run anywhere.

## install

```bash
pip install pyfuze
```

## usage

```bash
Usage: pyfuze [OPTIONS] PYTHON_PROJECT

  pyfuze — package Python scripts with dependencies.

Options:
  --python TEXT     Add .python-version file
  --reqs TEXT       Add requirements.txt file (input comma-separated string OR requirements.txt path)
  --pyproject FILE  Add pyproject.toml file
  --uv-lock FILE    Add uv.lock file
  --entry TEXT      Entry point (only works when PYTHON_PROJECT is a folder) [default: main.py]
  --include TEXT    Include additional file or folder (source[:destination], this option is repeatable)
  --win-gui         Enable Windows GUI mode (which hides the console window)
  -d, --debug       Enable debug logging
  -v, --version     Show the version and exit.
  -h, --help        Show this message and exit.
```

## example

```bash
pyfuze ./examples/ip.py --python 3.8 --reqs requests --win-gui
```

This command will generate `dist/ip.zip` which contains:

- pyfuze.com
- .python-version ("3.8")
- requirements.txt ("requests")
- src/ip.py
- config.txt (2 lines: "entry=ip.py" "win_gui=1")

`pyfuze.com` is an [Actually Portable Executable](https://justine.lol/ape.html), capable of running natively on Linux, macOS, Windows, FreeBSD, OpenBSD, NetBSD, and BIOS across both AMD64 and ARM64 architectures.

Executing `pyfuze.com` will do the following:

- install [uv](https://github.com/astral-sh/uv) in `./uv` folder
- install python in `./python` (version: `.python-version`)
- install dependencies in `requirements.txt`
- run the Python project

Note: uv primarily supports macOS (both Apple Silicon and x86_64), Linux (x86_64), and Windows (x86_64).
