import os

if __name__ == "__main__":
    if os.name == "nt":
        from windows_part import windows_main

        windows_main()
    elif os.name == "posix":
        from unix_part import unix_main

        unix_main()
    else:
        print("Not implemented")
