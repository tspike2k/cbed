//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.c"
#include "vulkan.c"
#include "display.h"

u8 g_memory[8*1024*1024];

int main(void){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    bool running = display_begin("Vulkan Test", 1024, 768, Display_Render_HW);
    while(running){
        Event event;
        while(display_next_event(&event)){
            switch(event.type){
                default: break;

                case Event_Type_Window_Close:{
                    running = false;
                } break;
            }
        }

        display_end_frame();
    }

    display_end();
    return 0;
}
