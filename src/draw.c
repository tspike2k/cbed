//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.h"
#include "draw.h"

#define Draw__Offsetof(T, member) ((size_t)&(((T*)0)->member))
#define Draw__Array_Length(a) ( (sizeof(a) / sizeof(a[0])) )

typedef struct Draw_State Draw_State;
static Draw_State draw__state;

enum{
    Draw_Cmd_Type_None,
    Draw_Cmd_Type_Quad,
};

// NOTE: The size field of the command header includes the size of the command header, the
// rest of the command data, and any additional data associated with it.
typedef struct{
    uint32_t type;
    uint32_t size;
} Draw_Cmd_Header;

typedef struct{
    Draw_Cmd_Header header;
    Draw_Texture    texture;
} Draw_Cmd_Quad;

typedef struct {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
    uint32_t color;
} Draw_Vertex;

typedef struct {
    Buffer           buffer;
    Draw_Cmd_Header *last_cmd;
} Draw_Layer;

typedef struct{
    bool        hw_rendering;
    Draw_Layer  layers[Draw_Layer_Total];
    Buffer      memory;
    uint32_t    layer_index;
    Vec2        solid_quad_uvs_min;
    Vec2        solid_quad_uvs_max;
} Draw_State_Common;

static Draw_Layer *draw__get_active_layer(){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    Draw_Layer *result = &s->layers[s->layer_index];
    return result;
}

static void draw__push_command(uint32_t cmd_type, uint32_t cmd_size){
    Draw_Layer *layer = draw__get_active_layer();
    Draw_Cmd_Header *header = (Draw_Cmd_Header *)buffer_push_bytes(&layer->buffer, cmd_size);
    header->type = cmd_type;
    header->size = cmd_size;

    layer->last_cmd = header;
}

Mat4_Pair orthographic_projection(Rect bounds, float n, float f){
    // Orthographic adapted from here:
    // https://songho.ca/opengl/gl_projectionmatrix.html#ortho
    // https://en.wikipedia.org/wiki/Orthographic_projection
    auto l = bounds.center.x - bounds.extents.x;
    auto r = bounds.center.x + bounds.extents.x;
    auto t = bounds.center.y + bounds.extents.y;
    auto b = bounds.center.y - bounds.extents.y;

    Mat4_Pair result;
    result.mat = (Mat4){{
        {2.0f / (r-l), 0,            0,             -(r+l)/(r-l)},
        {0,            2.0f / (t-b), 0,             -(t+b)/(t-b)},
        {0,            0,            -2.0f / (f-n), -(f+n)/(f-n)},
        {0,            0,            0,             1},
    }};

    result.inv = (Mat4){{
        {(r-l) / 2.0f, 0,            0,              (l+r)/2.0f},
        {0,            (t-b) / 2.0f, 0,              (t+b)/2.0f},
        {0,            0,            (f-n) / -2.0f, -(f+n)/2.0f},
        {0,            0,            0,             1},
    }};
    return result;
}

void draw_init_layer(uint32_t layer_id, size_t buffer_size){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;

    assert(layer_id < Draw_Layer_Total);
    Draw_Layer *layer = &s->layers[layer_id];
    Buffer *memory = &s->memory;
    assert(buffer_size <= memory->size - memory->used);
    layer->buffer = (Buffer){&memory->data[memory->used], buffer_size};
    memory->used += buffer_size;
}

uint32_t draw_set_layer(uint32_t layer_index){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    uint32_t result = s->layer_index;
    s->layer_index = layer_index;
    return result;
}

void draw_quad(float px, float py, float w, float h, uint32_t color){
    Draw_Layer *layer = draw__get_active_layer();

    // TODO: Use index buffer to use only 4 bytes
    size_t vertex_size = sizeof(Draw_Vertex)*6;
    if(!layer->last_cmd || layer->last_cmd->type != Draw_Cmd_Type_Quad){
        draw__push_command(Draw_Cmd_Type_Quad, sizeof(Draw_Cmd_Quad));
    }
    layer->last_cmd->size += vertex_size;

    Draw_Vertex *v = buffer_push_bytes(&layer->buffer, vertex_size);
    Vec3 p0 = {px + w, py}; // Top-right
    Vec3 p1 = {px, py}; // Top-left
    Vec3 p2 = {px, py - h}; // Bottom-left
    Vec3 p3 = {px + w, py - h}; // Bottom-right

    v[0].pos = p0;
    v[0].color = color;
    v[1].pos = p1;
    v[1].color = color;
    v[2].pos = p2;
    v[2].color = color;

    v[3].pos = p0;
    v[3].color = color;
    v[4].pos = p2;
    v[4].color = color;
    v[5].pos = p3;
    v[5].color = color;
}

//
// Font
//

Font_Glyph* get_glyph(Font* font, uint32_t codepoint){
    Font_Glyph* result = &font->null_glyph;
    for(uint32_t i = 0; i < font->glyphs_count; i++){
        if(font->glyph_codepoints[i] == codepoint){
            result = &font->glyphs[i];
            break;
        }
    }
    return result;
}

float font_get_kerning_advance(Font* font, uint32_t prev_codepoint, uint32_t codepoint){
    float result = 0;
    for(uint32_t i = 0; i < font->kerning_pairs_count; i++){
        Font_Kerning_Pair *entry = &font->kerning_pairs[i];
        if(entry->a == prev_codepoint && entry->b == codepoint){
            result = font->kerning_advance[i];
            break;
        }
    }
    return result;
}

static void *draw__read_bytes(Buffer *buffer, size_t bytes, bool *error){
    void *result = NULL;
    if(buffer->used + bytes <= buffer->size){
        result = &buffer->data[buffer->used];
        buffer->used += bytes;
    }
    else{
        *error = true;
    }

    return result;
}

static void *draw__read_bytes_expect_end(Buffer *buffer, size_t bytes, bool *error){
    void *result = draw__read_bytes(buffer, bytes, error);
    if(buffer->used != buffer->size){
        *error = true;
    }
    return result;
}

bool font_load_from_memory(Font* font, const char* font_name, void *memory, size_t memory_size){
    Buffer buffer = {memory, memory_size};
    bool error = false;

    Font_Header *header = (Font_Header*)draw__read_bytes(&buffer, sizeof(Font_Header), &error);
    if(!header){
        fmt_msg("Memory for font {0} is too short.\n", fmt_cstr(font_name));
        return false;
    }

    if(header->magic != Font_File_Magic){
        fmt_msg("Unexpected magic in file header for font {0}\n", fmt_cstr(font_name));
        return false;
    }

    if(header->version != Font_File_Version){
        fmt_msg(
            "Unsupported file version for font {0}. Expected {1} but got {2} instead.\n",
            fmt_cstr(font_name), fmt_i(Font_File_Version), fmt_i(header->version)
        );
        return false;
    }

    while(buffer.used < buffer.size && !error){
        Font_Section *section = (Font_Section*)draw__read_bytes(&buffer, sizeof(Font_Section), &error);
        if(!section){
            fmt_msg("Error! Unable to read next section for font {0}.\n", fmt_cstr(font_name));
            break;
        }

        size_t payload_size = section->size - sizeof(Font_Section);
        Buffer payload = {
            draw__read_bytes(&buffer, payload_size, &error),
            payload_size
        };
        if(!payload.data){
            fmt_msg("Error! Payload of section type {1} is too short for font {0}.\n", fmt_cstr(font_name), fmt_i(section->type));
            break;
        }

        switch(section->type){
            case Font_Section_Metrics:{
                font->metrics = (Font_Metrics*)draw__read_bytes_expect_end(&payload, sizeof(Font_Metrics), &error);
            } break;

            case Font_Section_Glyphs:{
                uint32_t *count = (uint32_t*)draw__read_bytes(&payload, sizeof(uint32_t), &error);
                if(count){
                    font->glyphs_count = *count;
                    font->glyph_codepoints = (uint32_t*)draw__read_bytes(
                        &payload, sizeof(uint32_t) * font->glyphs_count, &error
                    );
                    font->glyphs = (Font_Glyph*)draw__read_bytes_expect_end(
                        &payload, sizeof(Font_Glyph) * font->glyphs_count, &error
                    );
                }
            } break;

            case Font_Section_Kerning:
            case Font_Section_Pixels:
            case Font_Section_Blank_UVs:
                assert(0);
        }

        if(error){
            fmt_msg("Error! Section type {1} for font {0} is malformd.\n", fmt_cstr(font_name), fmt_i(section->type));
        }
    }

    return !error;
}

#ifdef __gnu_linux__
//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------

#include "opengl.h"

#define Draw__Constants_Uniform_Binding 0

typedef struct{
    Mat4     mat_camera;
    Vec3     camera_pos;
    float    time;
} Draw_Constants;

static const char *draw__default_vert =
"#version 330\n"
"layout(std140) uniform Constants{\n"
"    mat4  mat_camera;\n"
"    vec3  camera_pos; // TODO: Is there some way to do lighting without this?\n"
"    float time;\n"
"};\n"
"in vec3 v_pos;\n"
"in vec3 v_normal;\n"
"in vec2 v_uv;\n"
"in uint v_color;\n"
"\n"
"out vec2 f_uv;\n"
"out vec4 f_color;\n"
"\n"
"void main(){\n"
"    gl_Position = mat_camera*vec4(v_pos, 1);\n"
"    // For some reason, GLSL considers hex literals to be signed integers by default.\n"
"    // To do bit manipulation, we either need to cast hex literals to uint or append a.\n"
"    // 'u' to the end of the literal. How bizarre.\n"
"    float r = ((v_color >> 24) & 0xffu) / 255.0;\n"
"    float g = ((v_color >> 16) & 0xffu) / 255.0;\n"
"    float b = ((v_color >>  8) & 0xffu) / 255.0;\n"
"    float a = ((v_color >>  0) & 0xffu) / 255.0;\n"
"    f_color     = vec4(r, g, b, a);\n"
"    f_uv        = v_uv;\n"
"}\n";

static const char *draw__default_frag =
"#version 330\n"
"in vec2  f_uv;\n"
"in vec4  f_color;\n"
"out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = vec4(f_color.rgb*f_color.a, f_color.a);\n"
"}\n";

enum{
    Vertex_Attribute_ID_Pos,
    Vertex_Attribute_ID_Normal,
    Vertex_Attribute_ID_UV,
    Vertex_Attribute_ID_Color,
};

struct Draw_State{
    Draw_State_Common common;
    GLuint quad_vbo;
    GLuint default_shader;
    GLuint constants_ubo;
};

static Draw_State draw__state;

#define Draw__Indeces_Per_Quad 6

static void draw__debug_msg_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
GLsizei length, const GLchar *message, const void *userParam){
    if(severity != GL_DEBUG_SEVERITY_NOTIFICATION){
        fmt_msg_puts(message);
        fmt_msg_puts("\n");
    }
}

static GLuint draw__compile_shader_pass(GLenum pass_type, const char *shader_type_str, const char *source){
    GLuint shader = glCreateShader(pass_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE){
        char buffer[512];
        glGetShaderInfoLog(shader, 512, NULL, &buffer[0]);
        fmt_msg("Unable to compile {0}:\n{1}\n", fmt_cstr(shader_type_str), fmt_cstr(&buffer[0]));
        fmt_msg_puts(source);
        glDeleteShader(shader);
        shader = 0;
    }

    return shader;
}

static GLuint draw__compile_shader(const char *program_name, const char* vertex_source, const char *fragment_source){
    GLuint program = glCreateProgram();
    if(!program){
        fmt_msg("Unable to create shader {0}.\n", fmt_cstr(program_name));
        return 0;
    }

    glBindAttribLocation(program, Vertex_Attribute_ID_Pos,    "v_pos");
    glBindAttribLocation(program, Vertex_Attribute_ID_Normal, "v_normal");
    glBindAttribLocation(program, Vertex_Attribute_ID_UV,     "v_uv");
    glBindAttribLocation(program, Vertex_Attribute_ID_Color,  "v_color");

    GLuint vertex_shader = draw__compile_shader_pass(GL_VERTEX_SHADER, "Vertex Shader", vertex_source);
    if(!vertex_shader){
        fmt_msg("Unable to compile vertex shader for program {0}.\n", fmt_cstr(program_name));
        glDeleteProgram(program);
        return 0;
    }

    GLuint fragment_shader = draw__compile_shader_pass(GL_FRAGMENT_SHADER, "Fragment Shader", fragment_source);
    if(!fragment_shader){
        // TODO: Better logging function
        fmt_msg("Unable to compile fragment shader for program {0}.\n", fmt_cstr(program_name));
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    GLuint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, (GLint*)&link_status);
    if(link_status == GL_FALSE){
        char buffer[512];
        glGetProgramInfoLog(program, 512, NULL, &buffer[0]);
        fmt_msg("Unable to link shader:\n  {0}\n", fmt_cstr(&buffer[0]));
    }

    glDetachShader(program, fragment_shader);
    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    bool success = link_status != GL_FALSE;
    if(success){
        GLuint block_index = glGetUniformBlockIndex(program, "Constants");
        if(block_index != GL_INVALID_INDEX){
            glUniformBlockBinding(program, block_index, Draw__Constants_Uniform_Binding);
        }

        return program;
    }
    return 0;
}

#if 0
void destroy_shader(Shader* shader){
    if(shader.handle != 0){
        glDeleteProgram(shader.handle);
        shader.handle = 0;
        shader.uniform_loc_texture_diffuse = 0; // TODO: Is zero the correct null index? Or is it -1?
    }
}
#endif

bool draw_begin(void *memory, size_t memory_size){
    Draw_State *s = &draw__state;

    Display_Info info = display_get_info();
    s->common.hw_rendering = info.window_flags & Display_Flag_HW_Rendering;
    s->common.memory = (Buffer){memory, memory_size};

    bool success = true;
    if(s->common.hw_rendering){
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(draw__debug_msg_callback, NULL);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Using premultiplied alpha

        // According to Casey Muratori (Handmade Hero ep 372), driver vendors realized that
        // state changes through VAOs is actually quite inefficient.  So here, we set one once
        // and forget it.
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &s->quad_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s->quad_vbo);

        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Use premultiplied alpha

        glEnableVertexAttribArray(Vertex_Attribute_ID_Pos);
        glVertexAttribPointer(
            Vertex_Attribute_ID_Pos, 3, GL_FLOAT, GL_FALSE, sizeof(Draw_Vertex),
            (GLvoid*)Draw__Offsetof(Draw_Vertex, pos)
        );

        glEnableVertexAttribArray(Vertex_Attribute_ID_Normal);
        glVertexAttribPointer(
            Vertex_Attribute_ID_Normal, 3, GL_FLOAT, GL_FALSE, sizeof(Draw_Vertex),
            (GLvoid*)Draw__Offsetof(Draw_Vertex, normal)
        );

        glEnableVertexAttribArray(Vertex_Attribute_ID_UV);
        glVertexAttribPointer(
            Vertex_Attribute_ID_UV, 2, GL_FLOAT, GL_FALSE, sizeof(Draw_Vertex),
            (GLvoid*)Draw__Offsetof(Draw_Vertex, uv)
        );

        glEnableVertexAttribArray(Vertex_Attribute_ID_Color);
        glVertexAttribIPointer(
            Vertex_Attribute_ID_Color, 1, GL_UNSIGNED_INT, sizeof(Draw_Vertex),
            (GLvoid*)Draw__Offsetof(Draw_Vertex, color)
        );

        // Create the constants uniform buffer
        glGenBuffers(1, &s->constants_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, s->constants_ubo);
        glBindBufferRange(GL_UNIFORM_BUFFER, Draw__Constants_Uniform_Binding, s->constants_ubo, 0, sizeof(Draw_Constants));
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Draw_Constants), NULL, GL_DYNAMIC_DRAW); // TODO: Is static draw correct? We re-upload every frame.

        s->default_shader = draw__compile_shader("default", draw__default_vert, draw__default_frag);
    }
    return success;
}

void draw_end(){

}

void draw_frame_begin(){
    Draw_State *s = &draw__state;
    if(s->common.hw_rendering){
        Display_Info info = display_get_info();
        glViewport(0, 0, info.window_width, info.window_height);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        Draw_Constants constants = {};
        Rect bounds;

        float w = info.window_width;
        float h = info.window_height;
        float aspect_ratio = w / h;
        bounds.extents = (Vec2){0.5f*h*aspect_ratio, 0.5f*h};
        bounds.center = (Vec2){0, 0};
        Mat4_Pair mat_proj = orthographic_projection(bounds, -100, 100);

        constants.mat_camera = mat4_transpose(mat_proj.mat);
        /*constants.mat_camera = mat4_transpose(Mat4_Identity);*/

        glBindBuffer(GL_UNIFORM_BUFFER, s->constants_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(constants), &constants);
    }
}

static void draw__layer(Draw_State *s, Draw_Layer *layer){
    if(layer->buffer.used < sizeof(Draw_Cmd_Header)) return;
    assert(layer->buffer.data);

    Buffer *cmd_buffer = &layer->buffer;
    size_t cmd_cursor = 0;
    if(s->common.hw_rendering){
        while(cmd_cursor < cmd_buffer->used){
            Draw_Cmd_Header *header = (Draw_Cmd_Header*)&cmd_buffer->data[cmd_cursor];

            switch(header->type){
                default: assert(0);

                case Draw_Cmd_Type_Quad:{
                    size_t vertex_count = (header->size - sizeof(Draw_Cmd_Quad)) / sizeof(Draw_Vertex);
                    uint32_t total_vertex_size = vertex_count * sizeof(Draw_Vertex);

                    size_t cmd_payload_pos = cmd_cursor + sizeof(Draw_Cmd_Quad);
                    void *v = &cmd_buffer->data[cmd_payload_pos];

                    glUseProgram(s->default_shader);
                    glBindBuffer(GL_ARRAY_BUFFER, s->quad_vbo);
                    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(total_vertex_size), v, GL_DYNAMIC_DRAW);
                    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
                    /*glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->quad_index_buffer);*/
                    /*glDrawElements(GL_TRIANGLES, (GLsizei)(vertex_count * Draw__Indeces_Per_Quad), GL_UNSIGNED_INT, (GLvoid*)0);*/
                } break;
            }
            cmd_cursor += header->size;
        }
    }
}

void draw_frame_end(){
    Draw_State *s = &draw__state;
    for(uint32_t layer_index = Draw_Layer_None+1; layer_index < Draw_Layer_Total; layer_index++){
        Draw_Layer *layer = &s->common.layers[layer_index];
        draw__layer(s, layer);
        layer->buffer.used = 0;
        layer->last_cmd = NULL;
    }
}

//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------
#endif
