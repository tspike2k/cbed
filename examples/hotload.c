//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

// TODO: Make this actually hotload game code.

#define Ceabed_API __attribute__((visibility("default")))

#include "common.c"
#include "os.c"
#include "display.h"
#include "opengl.c"
#include "math.c"
#include "files.c"
#include "draw.c"

u8 g_memory[4*1024*1024];

typedef bool (*hotload_test_func)(Buffer *memory);

hotload_test_func hotload_test;

bool compile_and_load(File_Lib *app_lib){
    if(app_lib->flags & File_Flag_Is_Open){
        file_close_lib(app_lib);
    }

    // TODO: The only real issue with doing hotloading this way is it's impossible to see the output
    // of the system command. Is there a better way?
    system("gcc -g -Wall -Isrc -shared -nodefaultlibs -nostartfiles -o ./bin/libhotload.so examples/hotload_lib.c");
    if(file_open_lib(app_lib, "./bin/libhotload.so")){
        File_Load_Symbol(app_lib, hotload_test);
        return true;
    }
    else{
        return false;
    }
}

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Hotload", 1024, 768, display_flags)
        && draw_begin(&memory);

    File_Watcher file_watcher;
    file_watcher_begin(&file_watcher, buffer_push_bytes(&memory, 4096), 4096);
    file_watcher_add(&file_watcher, "./examples/hotload_lib.c");

    File_Lib app_lib = {};
    if(!compile_and_load(&app_lib)){
        running = false;
    }

    while(running){
        File_Watcher_Event watch_event;
        file_watcher_update(&file_watcher);
        while(file_watcher_next_event(&file_watcher, &watch_event)){
            if(watch_event.type == File_Watcher_Event_Modify){
                compile_and_load(&app_lib);
                fmt_msg_puts("Compiled and hotloaded hotload_lib.c.\n");
            }
        }

        running = hotload_test(&memory);
    }

    file_watcher_end(&file_watcher);

    draw_end();
    display_end();
    return 0;
}
