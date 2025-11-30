//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

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
    bool running = display_begin("Box", 1024, 768, display_flags)
        && draw_begin(&memory);

    const char *font_file_name = "./bin/font.fnt";
    String font_memory = file_read_into_memory(font_file_name, &memory);
    Font test_font;
    if(!font_load_from_memory(&test_font, font_file_name, font_memory.text, font_memory.size)){
        fmt_msg("Error loading font.\n");
    }

    draw_init_layer(Draw_Layer_World, 4096);
    while(running){
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

        /*Rect uvs = rect_from_min_max((Vec2){0, 0}, (Vec2){1, 1});*/

        draw_frame_begin();
        draw_set_layer(Draw_Layer_World);
#if 1
        draw_quad(rect_from_min_wh((Vec2){-100, -100}, 200, 200), 0x00ff00ff);
        draw_quad(rect_from_min_wh((Vec2){0, 0}, 200, 200), 0xff0000ff);
        draw_quad(rect_from_min_wh((Vec2){0, 0}, 100, 100), 0xffff00ff);
#endif

        const char *msg = "Hello, world!";
        draw_text((Vec2){0, 0}, 0xff00ffff, &test_font, msg, strlen(msg));

        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();
    }

    draw_end();
    display_end();
    return 0;
}
