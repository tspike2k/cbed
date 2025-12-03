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

#define Box_Color 0xff00ffff

// Cube mesh generated from obj file.
Draw_Vertex cube_mesh[3*12] = {
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.875000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.625000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.625000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.625000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.375000, 1.000000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.375000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.000000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.250000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.000000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.375000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.125000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.125000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.625000, 0.250000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.375000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.375000, 0.250000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.875000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.875000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.625000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.625000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.625000, 1.000000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.375000, 1.000000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.000000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.250000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.250000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.375000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.375000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.125000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.750000},
        .color  = Box_Color,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.625000, 0.250000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.625000, 0.500000},
        .color  = Box_Color,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.375000, 0.500000},
        .color  = Box_Color,
    },
};

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Box", 1024, 768, display_flags)
        && draw_begin(&memory);

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

        draw_frame_begin();
        draw_set_layer(Draw_Layer_World);

        Display_Info display = display_get_info();
        Vec3 screen_center = v3_muls((Vec3){display.window_width, display.window_height, 0}, 0.5f);

        Mat4 scale = mat4_scale((Vec3){50, 50, 50});
        Mat4 xform = mat4_mul(mat4_translate(screen_center), scale);
        draw_vertices(xform, &cube_mesh[0], Array_Len(cube_mesh));

        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();
    }

    draw_end();
    display_end();
    return 0;
}
