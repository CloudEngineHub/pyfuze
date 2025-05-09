import os
import subprocess


def run_pyfuze(*args):
    binary = os.path.join(os.path.dirname(__file__), "pyfuze.com")
    return subprocess.run([binary] + list(args))
