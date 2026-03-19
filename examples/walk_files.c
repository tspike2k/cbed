//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#include "common.c"
#include "files.c"

u8 memory[2048];

int main(){
    Buffer buffer = {memory, Array_Len(memory)};
    const char *exec_path = get_executable_path(&buffer);
    fmt_msg_puts(exec_path);
    fmt_msg_puts("\n");

    return 0;
}

