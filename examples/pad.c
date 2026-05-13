//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.c"
#include "display.h"
#include "opengl.c"
#include "math.c"
/*#include "files.c"*/
#include "draw.c"
#include "gamepad.c"

u8 g_memory[4*1024*1024];

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Box", 1024, 768, display_flags) && draw_begin(&memory);
    gamepad_begin(NULL, &memory);

    while(running){
        Event event;
        while(display_next_event(&event)){
            switch(event.type){
                default: break;

                case Event_Type_Window_Close:{
                    running = false;
                } break;

                case Event_Type_Mouse_Motion:{
                    Event_Mouse_Motion *motion = &event.mouse_motion;

                } break;

                case Event_Type_Key:{
                    Event_Key *key = &event.key;
                    running = key->id != Key_ID_Escape;
                } break;
            }
        }

        gamepad_update(&memory);
        if(gamepad_get_count() > 0){
            Gamepad_Event pad_evt;
            while(gamepad_poll(0, &pad_evt)){

            }
        }

        Display_Info display = display_get_info();
        draw_frame_begin();

        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();
    }

    gamepad_end();
    draw_end();
    display_end();
    return 0;
}
