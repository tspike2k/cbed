//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

// TODO: Make this actually hotload game code.

#include <stdbool.h>
#include "common.c"
#include "os.c"
#include "display.h"
#include "opengl.c"
#include "math.c"
#include "files.c"
#include "draw.c"

u8 g_memory[4*1024*1024];

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Hotload", 1024, 768, display_flags)
        && draw_begin(&memory);

    File_Watcher file_watcher;
    file_watcher_begin(&file_watcher, buffer_push_bytes(&memory, 4096), 4096);
    file_watcher_add(&file_watcher, "./examples/hotload.c");

    while(running){
        size_t memory_restore = buffer_frame_begin(&memory);

        Event event;
        while(display_next_event(&event)){
            switch(event.type){
                default: break;

                case Event_Type_Window_Close:{
                    running = false;
                } break;

                case Event_Type_Key:{
                    Event_Key *key = &event.key;
                    running = key->id != Key_ID_Escape;
                } break;
            }
        }

        File_Watcher_Event watch_event;
        file_watcher_update(&file_watcher);
        while(file_watcher_next_event(&file_watcher, &watch_event)){
            if(watch_event.type == File_Watcher_Event_Modify){
                fmt_msg_puts("hotload.c modified.\n");
            }
        }

        draw_frame_begin();
        draw_set_layer(Draw_Layer_World);

        Display_Info display = display_get_info();
        Vec2 window_center = v2_muls((Vec2){display.window_width, display.window_height}, 0.5f);

        draw_rect(rect_from_min_wh(v2_add(window_center, (Vec2){-100, -100}), 200, 200), 0xff00ff00);
        draw_rect(rect_from_min_wh(v2_add(window_center, (Vec2){0, 0}), 200, 200), 0xff0000ff);
        draw_rect(rect_from_min_wh(v2_add(window_center, (Vec2){0, 0}), 100, 100), 0xff00ffff);

        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();

        buffer_frame_end(&memory, memory_restore);
    }

    file_watcher_end(&file_watcher);

    draw_end();
    display_end();
    return 0;
}
