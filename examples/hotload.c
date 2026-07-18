//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
This is a demonstration of how to compile and reload a library each time the source file is
modified. Computers are so fast these days that small C application can compile quite fast,
so this work almost instantly for trivial applications. To see it in action, build the examples
using build_example.sh and then run ./bin/hotload from the project directory. While running,
try editing hotload_lib.c and then saving your changes. You could add new shapes, change the
color of the shapes displayed, whatever you wish. After saving the changes, you should see the
result quickly take effect.
*/

#include "common.c"
#include "os.c"
#include "display.h"
#include "opengl.c"
#include "math.c"
#include "files.c"
#include "font.c"
#include "draw.c"

u8 g_memory[4*1024*1024];

typedef bool (*hotload_test_func)(Buffer *memory);

hotload_test_func hotload_test;

typedef enum{
    Hotload_Success,
    Hotload_Compile_Fail,
    Hotload_Reload_Fail,
} Hotload_Result;

Hotload_Result compile_and_load(File_Lib *app_lib){
    Hotload_Result result = Hotload_Compile_Fail;
    const char *cmd = "gcc -g -Wall -Isrc -shared -nodefaultlibs -nostartfiles -o ./bin/libhotload.so examples/hotload_lib.c";
    FILE *f = popen(cmd, "r");
    if(f){
        // TODO: popen can return -1. Do error handling in that case.
        if(pclose(f) == 0){
            if(app_lib->flags & File_Flag_Is_Open){
                file_close_lib(app_lib);
            }

            if(file_open_lib(app_lib, "./bin/libhotload.so")){
                File_Load_Symbol(app_lib, hotload_test);
                result = Hotload_Success;
            }
            else{
                result = Hotload_Reload_Fail;
            }
        }
    }
    else{
        const char *msg = strerror(errno);
        fmt_msg("Unable to compile libhotload.so: {0}", fmt_cstr(msg));
    }

    return result;
}

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Hotload", 1024, 768, display_flags)
        && draw_begin(&memory);

    File_Watcher file_watcher;
    file_watcher_begin(&file_watcher, buffer_push_bytes(&memory, 4096), 4096);
    u32 hotload_lib_watcher = file_watcher_add(&file_watcher, "./examples/hotload_lib.c");

    File_Lib app_lib = {};
    if(compile_and_load(&app_lib) != Hotload_Success){
        running = false;
    }

    while(running){
        running = hotload_test(&memory);

        File_Watcher_Event watch_event;
        file_watcher_update(&file_watcher);
        while(file_watcher_next_event(&file_watcher, &watch_event)){
            if(watch_event.watch_id == hotload_lib_watcher
            && watch_event.type == File_Watcher_Event_Modify){
                // Should the code fail to compile, we can rely on the already loaded library
                // to continue to run correctly. Failure to reload the library, on the other
                // hand, is a fatal error. This is because the symbols loaded from the library
                // would become invalidated since we must unload it before trying (and failing)
                // to reload the library.
                Hotload_Result status = compile_and_load(&app_lib);
                if(status == Hotload_Success)
                    fmt_msg_puts("Compiled and hotloaded hotload_lib.c.\n");
                else if(status == Hotload_Reload_Fail)
                    running = false;
            }
        }
    }

    file_watcher_end(&file_watcher);

    draw_end();
    display_end();
    return 0;
}
