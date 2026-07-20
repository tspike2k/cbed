//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
While running ./bin/hotload, try editing this file and then saving the changes. The changes
should take effect rather quickly.

See hotload.c for a deeper explanation of this example.
*/

#define Ceabed_API extern
#include "common.h"
#include "draw.h"
#include "display.h"

Ceabed_API bool hotload_test(Buffer *memory){
    bool running = true;

    Buffer memory_restore = *memory;

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

    *memory = memory_restore;

    return running;
}
