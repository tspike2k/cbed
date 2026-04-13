/*
Authors:   tspike (github.com/tspike2k)
Copyright: Copyright (c) 2026
License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
*/

#include "common.c"
#include "os.c"
#include "display.h"
#include "opengl.c"
#include "math.c"
#include "files.c"
#include "draw.c"
#include "gui.c"

u8 g_memory[4*1024*1024];

enum{
    Gui_ID_None,
    Gui_ID_Btn_Test,
};

int main(){
    Gui gui = {};
    gui_init(&gui);

    Buffer memory = {g_memory, Array_Len(g_memory)};

    bool running = display_begin("Box", 1024, 768, Display_Flag_HW_Rendering)
        && draw_begin(&memory);

#   define Panel_Memory_Len 1*1024*1024
    Display_Info display = display_get_info();

    Gui_Def gui_def = gui_begin_def(&gui);
    Rect panel_bounds = {v2(20, 20), v2(200, 200)};
    gui_begin_panel(&gui_def, buffer_push_bytes(&memory, Panel_Memory_Len), Panel_Memory_Len, panel_bounds, 0);
    /*gui_button(&gui, Gui_ID_Btn_Test, str("Test"));*/
    gui_end_panel(&gui_def);
    gui_end_def(&gui_def);

    while(running){
        Event event;
        while(display_next_event(&event)){
            if(gui_handle_event(&gui, &event)) continue;

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

        draw_frame_begin();
        draw_set_layer(Draw_Layer_World);

        gui_draw(&gui);
        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();
    }

    draw_end();
    display_end();
    return 0;
}
