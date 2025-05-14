import platform
import tkinter as tk

import requests


def get_ip():
    try:
        response = requests.get("https://api.ipify.org")
        response.encoding = "utf-8"
        return response.text
    except Exception as e:
        return f"Error: {str(e)}"


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
