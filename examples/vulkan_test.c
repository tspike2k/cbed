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
#include "img.c"

u8 g_scratch[8*1024*1024];
u8 g_temp[8*1024*1024];
u8 g_memory[4*1024*1024];

#define Window_W 512
#define Window_H 512

typedef struct {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct{
    Mat4 projection;
    Mat4 view;
    Mat4 model[3]; // TODO: Why 3?
    Vec4 light_pos;
    u32 selected; // TODO: What is this for?
} Shader_Constants;

int main(void){
    Buffer memory  = {g_memory, Array_Len(g_memory)};
    Buffer temp    = {g_temp, Array_Len(g_temp)};
    Buffer scratch = {g_scratch, Array_Len(g_scratch)};

    bool running = display_begin("Vulkan Test", 1024, 768, Display_Flag_Hardware_Render);

    assert(sizeof(Shader_Constants) <= Constants_Buffer_Size);

    u32     mesh_vertices_count = 0;
    Vertex *mesh_vertices = NULL;
    u16    *mesh_indeces  = NULL;

    Obj_Data obj_data;
    if(obj_parse_file(&obj_data, "./bin/cube.obj", &temp, &scratch)){
        u32 vertices_per_face = 3;
        mesh_vertices_count = obj_data.faces_count * vertices_per_face;
        mesh_vertices = buffer_push_array(Vertex, &memory, mesh_vertices_count);

        // TODO: Are these the correct indeces for an obj file? I think so.
        mesh_indeces = buffer_push_array(u16, &memory, mesh_vertices_count);
        for_count(u32, i, mesh_vertices_count){
            mesh_indeces[i] = i;
        }

        for_count(u32, face_index, obj_data.faces_count){
            Obj_Polygon polygon = obj_polygon_from_face(&obj_data, face_index);

            assert(polygon.flags & (Obj_Flag_Texture | Obj_Flag_Normal));
            assert(polygon.vertex_count == vertices_per_face);

            for_count(u32, v_index, vertices_per_face){
                Obj_Vertex *src = &polygon.vertex[v_index];
                Vertex *dest = &mesh_vertices[face_index * vertices_per_face + v_index];

                dest->pos    = v3(src->position.x, src->position.y, src->position.z);
                dest->normal = src->normal;
                dest->uv     = v2(src->texture.x, src->texture.y);
            }
        }
    }
    else{
        fmt_msg_puts("ERROR: Unable to load obj file.\n");
        running = false;

    };

    u32 mesh_vertex_buffer_size = sizeof(Vertex) * mesh_vertices_count;
    u32 mesh_index_buffer_size  = sizeof(u16) * mesh_vertices_count;
    u32 mesh_buffer_size = mesh_vertex_buffer_size + mesh_index_buffer_size;

    VkBufferUsageFlags mesh_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

#if 0
    // TODO: Should we call this vulkan_create_mapped_buffer?
    Vulkan_Buffer mesh_buffer;
    if(running && vulkan_create_buffer(mesh_buffer_size, mesh_usage, &mesh_buffer)){
        memcpy(mesh_buffer.mapped_ptr, mesh_vertices, mesh_vertex_buffer_size);
        void *index_buffer_memory = mesh_buffer.mapped_ptr + mesh_vertex_buffer_size;
        memcpy(index_buffer_memory, mesh_indeces, mesh_index_buffer_size);
    }
    else{
        running = false;
    }

    Vulkan_Image texture;
    if(running){
        Img_Pixels pixels = img_load_tga("./bin/test.tga", &temp, &scratch);
        if(pixels.data){
            if(!vulkan_load_texture(pixels.data, pixels.width, pixels.height, &texture)){
                running = false;
            }
        }
        else{
            running = false;
        }
    }
#endif

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

        vulkan_draw_test_triangle();

        //display_flip_backbuffer();
        display_end_frame();
    }
    display_end();
    return 0;
}
