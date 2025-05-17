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
pyfuze ./examples/ip.py --reqs requests --win-gui
```

This command will generate `dist/ip.zip` which contains:

- pyfuze.com
- .python-version (default "3.8")
- requirements.txt ("requests")
- src/ip.py
- config.txt (2 lines: entry=ip.py win_gui=1)

`pyfuze.com` is an [Actually Portable Executable](https://justine.lol/ape.html), capable of running natively on Linux, macOS, Windows, FreeBSD, OpenBSD, NetBSD, and BIOS across both AMD64 and ARM64 architectures.

Executing `pyfuze.com` will do the following:

- install [uv](https://github.com/astral-sh/uv) in `./uv` folder
- install python in `./python` (version: `.python-version`)
- install dependencies in `requirements.txt`
- run the Python project

Note: uv primarily supports macOS (both Apple Silicon and x86_64), Linux (x86_64), and Windows (x86_64).
