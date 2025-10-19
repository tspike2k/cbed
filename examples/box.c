//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.c"
#include "display.c"
#include <stdbool.h>

#define MIN(a, b) (a) < (b) ? (a) : (b);
#define MAX(a, b) (a) > (b) ? (a) : (b);

static void draw_rect(Display_Backbuffer *buffer, int x, int y, int w, int h, uint32_t color){
    uint32_t min_x = MAX(0, x);
    uint32_t min_y = MAX(0, y);
    uint32_t max_x = MAX(0, x + w);
    uint32_t max_y = MAX(0, y + h);

    max_x = MIN(max_x, buffer->width);
    max_y = MIN(max_y, buffer->height);

    for(uint32_t y = min_y; y < max_y; y++){
        for(uint32_t x = min_x; x < max_x; x++){
            buffer->pixels[x + y * buffer->width] = color;
        }
    }
}

int main(){
    bool running = display_begin("Box", 1024, 768, 0);
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

        Display_Backbuffer buffer = display_get_sw_backbuffer();
        draw_rect(&buffer, 20, 20, 200, 120, 0xff00ffff);

        display_flip_backbuffer();
        display_end_frame();
    }

    display_end();
    return 0;
}
