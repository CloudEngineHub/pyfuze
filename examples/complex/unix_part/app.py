import os
import platform

import distro


def unix_main():
    print(f"System: {platform.system()}")
    print(f"Machine: {platform.machine()}")
    print(f"Version: {platform.version()}")
    print(f"Distro: {distro.name(pretty=True)}")
    print(f"Name: {distro.codename().capitalize()}")
    print(f"Desktop: {os.environ.get('XDG_SESSION_TYPE').capitalize()}")
