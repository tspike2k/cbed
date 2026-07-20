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
    Gui gui = {0};
    gui_init(&gui);

    Buffer memory = {g_memory, Array_Len(g_memory)};

    bool running = display_begin("Box", 1024, 768, Display_Flag_HW_Rendering)
        && draw_begin(&memory);

    const char *font_file_name = "./bin/font.fnt";
    String font_memory = file_read_into_memory(font_file_name, &memory);
    Font font;
    if(!font_load_from_memory(&font, font_file_name, font_memory.text, font_memory.size)){
        fmt_msg("Error loading font.\n");
    }

    gui.font = &font;

#   define Panel_Memory_Len 1*1024*1024

    Gui_Panel *panel = gui_begin_panel(&gui, buffer_push_bytes(&memory, Panel_Memory_Len), Panel_Memory_Len, 0);
    panel->target_size = v2(1.0f, 0);
    /*gui_begin_menu_bar(&gui);*/
    /*gui_end_menu_bar(&gui);*/

    /*gui_button(&gui, Gui_ID_Btn_Test, str("Test"));*/
    gui_end_panel(panel);

    panel = gui_begin_panel(
        &gui, buffer_push_bytes(&memory, Panel_Memory_Len), Panel_Memory_Len,
        Gui_Panel_Flag_Floating);
    gui_end_panel(panel);

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

        Display_Info display = display_get_info();

        gui_update(&gui, display.window_width, display.window_height, 0);

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
