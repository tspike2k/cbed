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
    buffer.used--; // Rewind enough to write over the null-terminator
    buffer_put(&buffer, "../", 3);
    buffer_null_terminate(&buffer);

    File_Walker walker;
    file_walker_begin(&walker, exec_path);
    while(file_walker_advance(&walker)){
        fmt_msg("{0}\n", fmt_cstr(walker.file_name));
        if(walker.file_type == File_Type_Directory){
            /*file_walker_enter_directory(&walker);*/
        }
    }
    file_walker_end(&walker);

    return 0;
}

