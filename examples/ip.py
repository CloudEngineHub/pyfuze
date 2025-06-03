import platform
import tkinter as tk

import requests


def get_ip():
    try:
        response = requests.get("https://4.ipw.cn", timeout=3)
        return response.text.strip()
    except Exception as e:
        return f"Error: {e}"


def main():
    ip = get_ip()

    if platform.system() == "Windows":
        root = tk.Tk()
        root.title(f"Hello pyfuze! {ip=}")
        root.geometry("300x200")
        root.mainloop()
    else:
        print(f"IP: {ip}")


if __name__ == "__main__":
    main()
