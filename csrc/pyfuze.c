/*
This script will be compiled using cosmocc

License from the https://github.com/jart/cosmopolitan project:

ISC License

Copyright 2020 Justine Alexandra Roberts Tunney

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted, provided that the
above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "libc/dce.h"

#include "pyfuze_utils.h"
#include "pyfuze_windows.h"
#include "pyfuze_unix.h"

extern char **environ;

// NOTE: Cosmopolitan linker script is hard-coded to change the
// subsystem from TUI to GUI when GetMessage() is defined.
void GetMessage() {}

int main(int argc, char *argv[]) {
    chdir_to_executable_folder(argv);
    read_config();
    if (IsWindows()) {
        return main_windows(argc, argv);
    } else {
        return main_unix(argc, argv);
    }
}
