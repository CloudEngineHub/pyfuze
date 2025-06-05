def set_pe_subsystem(file_path: str, subsystem_type: int):
    with open(file_path, "rb+") as f:
        # Read e_lfanew to find PE header location
        f.seek(0x3C)
        e_lfanew = int.from_bytes(f.read(4), byteorder="little")

        # Calculate subsystem field offset (PE Header + 92)
        subsystem_offset = e_lfanew + 92

        # Seek to subsystem field and write new value
        f.seek(subsystem_offset)

        # 0x02: GUI, 0x03: Console
        subsystem_bytes = subsystem_type.to_bytes(2, byteorder="little")
        f.write(subsystem_bytes)


# By default, pyfuze.com uses the GUI subsystem
# def set_pe_subsystem_gui(file_path: str):
#     return set_pe_subsystem(file_path, 0x02)


def set_pe_subsystem_console(file_path: str):
    return set_pe_subsystem(file_path, 0x03)
