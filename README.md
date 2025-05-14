# pyfuze

[![GitHub](https://img.shields.io/badge/GitHub-5c5c5c)](https://github.com/TanixLu/pyfuze)
[![PyPI - Version](https://img.shields.io/pypi/v/pyfuze)](https://pypi.org/project/pyfuze/)

## description

pyfuze makes your python script actually run anywhere.

## install

```bash
pip install pyfuze
```

## usage

```bash
pyfuze ./examples/ip.py --python 3.8 --reqs requests --win-gui
```

This command will generate `dist/ip.zip` which contains:

- pyfuze.com
- .python-version ("3.8")
- requirements.txt ("requests")
- t.py
- WIN_GUI (an empty file used as a flag)

`pyfuze.com` is an [actually portable executable](https://justine.lol/ape.html), so you can run it natively on Linux + Mac + Windows + FreeBSD + OpenBSD + NetBSD + BIOS on AMD64 and ARM64.

Notice: uv mainly supports macOS (Apple Silicon + x86_64), Linux (x86_64) and Windows (x86_64).

Executing `pyfuze.com` will result in:

- install uv in `./uv` folder
- install python in `./python` (version: `.python-version`)
- install dependencies in `requirements.txt`
- run python script
