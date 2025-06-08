import sys
import datetime

print("Hello pyfuze!")

print(f"{sys.argv=}")

print(f"{sys.path=}")

print(f"{sys.executable=}")

print(f"{sys.platform=}")

print(f"{sys.version=}")

print(f"{datetime.datetime.now()=}")

try:
    import requests

    ip = requests.get("https://4.ipw.cn").text
    print(f"{ip=}")
except Exception as e:
    print(f"{e=}")

input("Press Enter to continue...")
