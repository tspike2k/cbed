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
#include "font.c"
#include "files.c"
#include "draw.c"

u8 g_memory[4*1024*1024];

// Cube mesh generated from obj file.
Draw_Vertex cube_mesh[3*12] = {
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.875000, 0.500000},
        .color  = 0xff000000,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.625000, 0.750000},
        .color  = 0xff000000,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.625000, 0.500000},
        .color  = 0xff000000,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.625000, 0.750000},
        .color  = 0xffff00ff,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.375000, 1.000000},
        .color  = 0xffff00ff,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.375000, 0.750000},
        .color  = 0xffff00ff,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.000000},
        .color  = 0xff00ff00,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.250000},
        .color  = 0xff00ff00,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.000000},
        .color  = 0xff00ff00,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.375000, 0.500000},
        .color  = 0xff00ffff,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.125000, 0.750000},
        .color  = 0xff00ffff,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.125000, 0.500000},
        .color  = 0xff00ffff,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.500000},
        .color  = 0xffff0000,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.750000},
        .color  = 0xffff0000,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.500000},
        .color  = 0xffff0000,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.625000, 0.250000},
        .color  = 0xff0000ff,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.375000, 0.500000},
        .color  = 0xff0000ff,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.375000, 0.250000},
        .color  = 0xff0000ff,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.875000, 0.500000},
        .color  = 0xff000000,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.875000, 0.750000},
        .color  = 0xff000000,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, 1.000000, -0.000000},
        .uv     = {0.625000, 0.750000},
        .color  = 0xff000000,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.625000, 0.750000},
        .color  = 0xffff00ff,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.625000, 1.000000},
        .color  = 0xffff00ff,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -0.000000, 1.000000},
        .uv     = {0.375000, 1.000000},
        .color  = 0xffff00ff,
    },
    {
        .pos =    {-0.500000, 0.500000, 0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.000000},
        .color  = 0xff00ff00,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.250000},
        .color  = 0xff00ff00,
    },
    {
        .pos =    {-0.500000, -0.500000, -0.500000},
        .normal = {-1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.250000},
        .color  = 0xff00ff00,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.375000, 0.500000},
        .color  = 0xff00ffff,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.375000, 0.750000},
        .color  = 0xff00ffff,
    },
    {
        .pos =    {-0.500000, -0.500000, 0.500000},
        .normal = {-0.000000, -1.000000, -0.000000},
        .uv     = {0.125000, 0.750000},
        .color  = 0xff00ffff,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.500000},
        .color  = 0xffff0000,
    },
    {
        .pos =    {0.500000, 0.500000, 0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.625000, 0.750000},
        .color  = 0xffff0000,
    },
    {
        .pos =    {0.500000, -0.500000, 0.500000},
        .normal = {1.000000, -0.000000, -0.000000},
        .uv     = {0.375000, 0.750000},
        .color  = 0xffff0000,
    },
    {
        .pos =    {-0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.625000, 0.250000},
        .color  = 0xff0000ff,
    },
    {
        .pos =    {0.500000, 0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.625000, 0.500000},
        .color  = 0xff0000ff,
    },
    {
        .pos =    {0.500000, -0.500000, -0.500000},
        .normal = {-0.000000, -0.000000, -1.000000},
        .uv     = {0.375000, 0.500000},
        .color  = 0xff0000ff,
    },
};

static void set_world_projection(Camera* camera, float width, float height, float z_near, float z_far){
    Vec2 camera_extents = {width*0.5f, height*0.5f};
    Rect camera_bounds = {camera_extents, camera_extents};
    camera->proj = orthographic_projection(camera_bounds, z_near, z_far);
}

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Box", 1024, 768, display_flags)
        && draw_begin(&memory);

    Vec3 camera_polar = (Vec3){90, -45, 1}; // TODO: Make these in radian eventually

    {
        auto m = make_lookat_matrix((Vec3){0, 0, -20}, (Vec3){0, 20, 0}, (Vec3){0, 1, 0});
        int i =42;
    }

    float t = 0;

    f32 dt = 2.0f; // TODO: Actually do frame timing.
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
                    camera_polar.x += motion->rel_x * dt;
                    camera_polar.y += motion->rel_y * dt;
                    clampf(&camera_polar.y, -78.75f, 78.75f);
                } break;

                case Event_Type_Key:{
                    Event_Key *key = &event.key;
                    running = key->id != Key_ID_Escape;
                } break;
            }
        }

        Display_Info display = display_get_info();
        Vec3 screen_center = v3_muls((Vec3){display.window_width, display.window_height, 0}, 0.5f);
        Vec3 cube_pos = v3_add(screen_center, (Vec3){0, 0, 0});

        t += dt;

        Camera camera = {0};
        float z_near = -1000.0f;
        float z_far =   1000.0f;
        set_world_projection(&camera, display.window_width, display.window_height, z_near, z_far);
        camera.center = (Vec3){0, 0, 0}; // TODO: Is this the correct center?
        camera.facing = (Vec3){0, 0, 1}; // TODO: Should z be -1?
        camera.view = camera_view_from_polar(camera_polar, cube_pos, (Vec3){0, 1, 0});
        // Center the camera by translating by half the camera size in world coordinates
        // (here they're the same as screen coordinates).
        camera.view.mat = mat4_mul(mat4_translate(v3_muls(screen_center, 1)), camera.view.mat);
        camera.view.inv = invert_view_matrix(camera.view.mat);

        draw_frame_begin();
        draw_set_layer(Draw_Layer_World);
        draw_set_culling(z_near, z_far);

        Mat4 scale = mat4_scale((Vec3){100, 200, 100});
        Mat4 xform = mat4_mul(mat4_translate(cube_pos), scale);
        draw_set_shader_3D();
        draw_set_camera(&camera);
        draw_vertices(xform, &cube_mesh[0], Array_Len(cube_mesh));

        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();
    }

    draw_end();
    display_end();
    return 0;
}
