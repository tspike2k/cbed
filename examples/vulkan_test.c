//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
    IMPORTANT: If enabling debugging for Vulkan, make sure the validation layers are installed!
    This can be done in arch using the following command:
        sudo pacman -S vulkan-validation-layers
*/

#include "common.c"
#include "vulkan.c"
/*#include "display.h"*/

u8 g_memory[8*1024*1024];

int main(void){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    Display *display = XOpenDisplay(NULL);
    if(display){
        int screen = DefaultScreen(display);
        Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 512, 512, 0, 0, 0);
        XMapWindow(display, window);
        XSync(display, True);

        if(window){
            if(vulkan_init(display, window, "Vulkan Test", true))
                fmt_msg_puts("Initialized Vulkan!\n");
        }
        else{
            fmt_msg("ERROR: Unable to create Xlib window!\n");
        }
    }
    else{
        fmt_msg("ERROR: Unable to create Xlib display!\n");
    }

    vulkan_destroy();

    return 0;
}
