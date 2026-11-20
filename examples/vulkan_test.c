//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

// To build, use the following:
// gcc -g examples/vulkan_test.c -I src -lvulkan -lX11

/*
    IMPORTANT: If enabling debugging for Vulkan, make sure the validation layers are installed!
    This can be done in arch using the following command:
        sudo pacman -S vulkan-validation-layers
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <stdlib.h> // free
#include "common.c"
#define Display_Vulkan
#include "display.c"
#include "vulkan.h"
#include "files.c"
#include "obj.c"

u8 g_scratch[8*1024*1024];
u8 g_temp[8*1024*1024];
u8 g_memory[4*1024*1024];

#define Window_W 512
#define Window_H 512

int main(void){
    Buffer memory  = {g_memory, Array_Len(g_memory)};
    Buffer temp    = {g_temp, Array_Len(g_temp)};
    Buffer scratch = {g_scratch, Array_Len(g_scratch)};

    bool running = display_begin("Vulkan Test", 1024, 768, 0);

    Obj_Data obj_data;
    if(!obj_parse_file(&obj_data, "./bin/cube.obj", &temp, &scratch)){
        fmt_msg_puts("ERROR: Unable to load obj file.\n");
        running = false;
    };

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

        //display_flip_backbuffer();
        display_end_frame();
    }
    display_end();
    return 0;
}



#if 0
int main(void){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    Display *display = XOpenDisplay(NULL);
    if(display){
        int screen = DefaultScreen(display);
        Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, Window_W, Window_H, 0, 0, 0);
        XMapWindow(display, window);
        XSync(display, True);

        if(window){
            if(vulkan_init(display, window, Window_W, Window_H, "Vulkan Test", true)){
                fmt_msg_puts("Initialized Vulkan!\n");


                vulkan_destroy();
            }
        }
        else{
            fmt_msg("ERROR: Unable to create Xlib window!\n");
        }
    }
    else{
        fmt_msg("ERROR: Unable to create Xlib display!\n");
    }

    return 0;
}
#endif
