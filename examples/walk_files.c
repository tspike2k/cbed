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
    const char *root_path = get_executable_path(&buffer);
    buffer.used--; // Rewind enough to write over the null-terminator
    buffer_put_text(&buffer, "..", 3);
    buffer_null_terminate(&buffer);

    /*fmt_msg_puts(root_path);*/
    /*fmt_msg_puts("\n");*/

    File_Walker walker;
    file_walker_begin(&walker, root_path);
    while(file_walker_advance(&walker)){
        size_t restore = buffer_frame_begin(&buffer);

        String s = file_walker_make_path(&walker, &buffer);
        fmt_msg("{0}\n", fmt_cstr(s.text));
        if(walker.file_type == File_Type_Directory){
            if(strcmp(walker.file_name, ".git") != 0){
                file_walker_enter_directory(&walker);
            }
        }

        buffer_frame_end(&buffer, restore);
    }
    file_walker_end(&walker);

    return 0;
}

