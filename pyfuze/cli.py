import sys

from . import run_pyfuze


def main():
    sys.exit(run_pyfuze(*sys.argv[1:]).returncode)
