//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.h"
#include "draw.h"

// TODO: We should have a #define that states what rendering backend we're targeting.
// If it's not defined, we select the best one for the given system.

#define Draw__Offsetof(T, member) ((size_t)&(((T*)0)->member))
#define Draw__Array_Length(a) ( (sizeof(a) / sizeof(a[0])) )

#define Draw_Texture_Null 0

typedef struct Draw_State Draw_State;
static Draw_State draw__state;

// TODO: Use index buffer so we only use 4 vertices
#define Draw__Vertex_Per_Quad 6

enum{
    Draw_Cmd_Type_None,
    Draw_Cmd_Type_Quad,
    Draw_Cmd_Type_Vertices,
    Draw_Cmd_Type_Set_Shader,
};

typedef struct Draw_Cmd Draw_Cmd;

#define Draw_Cmd_Fields \
    Draw_Cmd *next;     \
    u32       type;

struct Draw_Cmd{
    Draw_Cmd_Fields;
};

typedef struct{
    Draw_Cmd_Fields;
    Draw_Texture texture;
    u32          vertex_count;
} Draw_Cmd_Quad;

typedef struct{
    Draw_Cmd_Fields;
    Mat4         xform;
    Draw_Vertex *vertices;
    size_t       vertices_count;
} Draw_Cmd_Vertices;

typedef struct{
    Draw_Cmd_Fields;
    Draw_Shader shader;
} Draw_Cmd_Set_Shader;

typedef struct {
    Camera   *camera;
    Draw_Cmd *cmd_first;
    Draw_Cmd *cmd_last;
} Draw_Layer;

typedef struct{
    bool         hw_rendering;
    Draw_Layer   layers[Draw_Layer_Total];
    Buffer      *memory;
    u32          layer_index;
    Vec2         solid_quad_uvs_min;
    Vec2         solid_quad_uvs_max;
    Draw_Texture blank_texture;
    Camera       default_camera;
    Draw_Shader  shader_default_2d;
    Draw_Shader  shader_default_3d;
} Draw_State_Common;

static Draw_Layer *draw__get_active_layer(){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    Draw_Layer *result = &s->layers[s->layer_index];
    return result;
}

static Draw_Cmd *draw__push_command(Draw_Layer *layer, u32 cmd_type, u32 cmd_size){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    Draw_Cmd *cmd = (Draw_Cmd *)buffer_push_bytes(s->memory, cmd_size);

    cmd->type = cmd_type;
    if(!layer->cmd_first)
        layer->cmd_first = cmd;

    if(layer->cmd_last)
        layer->cmd_last->next = cmd;
    layer->cmd_last = cmd;
    return cmd;
}

Draw_XForm orthographic_projection(Rect bounds, float n, float f){
    // Orthographic adapted from here:
    // https://songho.ca/opengl/gl_projectionmatrix.html#ortho
    // https://en.wikipedia.org/wiki/Orthographic_projection
    f32 l = bounds.center.x - bounds.extents.x;
    f32 r = bounds.center.x + bounds.extents.x;
    f32 t = bounds.center.y + bounds.extents.y;
    f32 b = bounds.center.y - bounds.extents.y;

    Draw_XForm result;
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

Draw_XForm camera_view_from_polar(Vec3 camera_polar, Vec3 camera_target, Vec3 up){
    Draw_XForm result;
    Vec3 camera_world = polar_to_world(camera_polar, camera_target);
    result.mat = make_lookat_matrix(camera_world, camera_target, up);
    result.inv = invert_view_matrix(result.mat);

    return result;
}

Mat4 make_lookat_matrix(Vec3 camera_pos, Vec3 look_pos, Vec3 up_pos){
    Vec3 look_dir = v3_normalize(v3_sub(look_pos, camera_pos));
    Vec3 up_dir   = up_pos;

    Vec3 right_dir   = v3_normalize(cross(look_dir, up_dir));
    Vec3 perp_up_dir = cross(right_dir, look_dir);

    Mat4 result = {{
        {right_dir.x, perp_up_dir.x, -look_dir.x, 0},
        {right_dir.y, perp_up_dir.y, -look_dir.y, 0},
        {right_dir.z, perp_up_dir.z, -look_dir.z, 0},
        {0,           0,             0,           1},
    }};

    camera_pos = v3_muls(camera_pos, -1);
    result = mat4_mul(mat4_transpose(result), mat4_translate(camera_pos));
    return result;
}

Mat4 invert_view_matrix(Mat4 view){
    // IMPORTANT: Inverting a view matrix this way only works if no non-uniform rotation has been
    // applied.

    // Transpose 3x3 rotation portion of the view to invert it.
    Mat4 rot = {{
        {view.m[0][0], view.m[1][0], view.m[2][0], 0},
        {view.m[0][1], view.m[1][1], view.m[2][1], 0},
        {view.m[0][2], view.m[1][2], view.m[2][2], 0},
        {0,            0,            0,            1},
    }};

    // Negate the translation portion of the view to invert it.
    Vec3 p = {
        -view.m[0][3],
        -view.m[1][3],
        -view.m[2][3]
    };
    Mat4 result = mat4_mul(rot, mat4_translate(p));
    return result;
}

Rect calc_scaling_viewport(float res_x, float res_y, float window_w, float window_h){
    // TODO(tspike) Investigate why there's a vertical stripe of non-rendered pixels on the right side of the screen when
    // in fullcreen mode in X11 (on my laptop). Floating point precision issues? Do we need to clamp?

    // Resolution independent code thanks to this post:
    // http://www.david-amador.com/2013/04/opengl-2d-independent-resolution-rendering/

    float aspect_ratio = res_x/res_y;

    float target_w = window_w;
    float target_h = target_w / aspect_ratio;

    if(target_h > window_h){
        target_w = target_h*aspect_ratio;
        target_h = window_h;
    }

    Vec2 min_p = {
        (window_w - target_w)*0.5f,
        (window_h - target_h)*0.5f
    };

    Rect result = rect_from_min_wh(min_p, target_w, target_h);
    return result;
}

u32 draw_set_layer(u32 layer_index){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    u32 result = s->layer_index;
    s->layer_index = layer_index;
    return result;
}

static void draw__set_quad(Draw_Vertex* v, Rect bounds, u32 color, Rect uvs){
    Vec2 r_min = rect_min(bounds);
    Vec2 r_max = rect_max(bounds);
    Vec2 uv_min = rect_min(uvs);
    Vec2 uv_max = rect_max(uvs);

    Vec3 p0 = {r_max.x, r_max.y}; // Top-right
    Vec3 p1 = {r_min.x, r_max.y}; // Top-left
    Vec3 p2 = {r_min.x, r_min.y}; // Bottom-left
    Vec3 p3 = {r_max.x, r_min.y}; // Bottom-right

    assert(Draw__Vertex_Per_Quad == 6);
    v[0].pos = p0;
    v[0].color = color;
    v[0].uv   = (Vec2){uv_max.x, uv_min.y};
    v[1].pos = p1;
    v[1].color = color;
    v[1].uv   = (Vec2){uv_min.x, uv_min.y};
    v[2].pos = p2;
    v[2].color = color;
    v[2].uv   = (Vec2){uv_min.x, uv_max.y};

    v[3].pos = p0;
    v[3].color = color;
    v[3].uv   = (Vec2){uv_max.x, uv_min.y};
    v[4].pos = p2;
    v[4].color = color;
    v[4].uv   = (Vec2){uv_min.x, uv_max.y};
    v[5].pos = p3;
    v[5].color = color;
    v[5].uv   = (Vec2){uv_max.x, uv_max.y};
}

static Draw_Vertex *draw__add_quad_cmd(Draw_Layer* layer, Draw_Texture texture, size_t vertex_count){
    Draw_Cmd_Quad *cmd = (Draw_Cmd_Quad *)draw__push_command(layer, Draw_Cmd_Type_Quad, sizeof(Draw_Cmd_Quad) + sizeof(Draw_Vertex)*vertex_count);
    cmd->texture = texture;
    cmd->vertex_count = vertex_count;
    Draw_Vertex *result = (Draw_Vertex*)(cmd+1);
    return result;
}

void draw_rect_textured(Rect r, u32 color, Draw_Texture texture, Rect uvs){
    Draw_Layer *layer = draw__get_active_layer();
    Draw_Vertex *v = draw__add_quad_cmd(layer, texture, Draw__Vertex_Per_Quad);
    draw__set_quad(v, r, color, uvs);
}

void draw_rect(Rect r, u32 color){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    Rect uvs = rect_from_min_max((Vec2){0, 0}, (Vec2){1, 1});
    draw_rect_textured(r, color, s->blank_texture, uvs);
}

void draw_vertices(Mat4 xform, Draw_Vertex *v, size_t vertex_count){
    Draw_Layer *layer = draw__get_active_layer();
    Draw_Cmd_Vertices *cmd = (Draw_Cmd_Vertices*)draw__push_command(layer, Draw_Cmd_Type_Vertices, sizeof(Draw_Cmd_Vertices));
    cmd->xform = xform;
    cmd->vertices = v;
    cmd->vertices_count = vertex_count;
}

void draw_set_shader_3D(){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    Draw_Layer *layer = draw__get_active_layer();
    Draw_Cmd_Set_Shader *cmd = (Draw_Cmd_Set_Shader*)draw__push_command(
        layer, Draw_Cmd_Type_Set_Shader, sizeof(Draw_Cmd_Set_Shader)
    );
    cmd->shader = s->shader_default_3d;
}

void draw_set_shader_2D(){
    Draw_State_Common *s = (Draw_State_Common*)&draw__state;
    Draw_Layer *layer = draw__get_active_layer();
    Draw_Cmd_Set_Shader *cmd = (Draw_Cmd_Set_Shader*)draw__push_command(
        layer, Draw_Cmd_Type_Set_Shader, sizeof(Draw_Cmd_Set_Shader)
    );
    cmd->shader = s->shader_default_2d;
}

void draw_text(Vec2 baseline, u32 color, Font *font, const char* text, size_t text_len){
    if(font->glyphs_count == 0) return;

    // TODO: Some of these characters won't be rendered (such as space).
    // Shouldn't we subtract them from the layer's vertex buffer and the
    // command's total size?
    Draw_Layer *layer = draw__get_active_layer();
    Draw_Vertex *v = draw__add_quad_cmd(layer, font->texture, text_len*Draw__Vertex_Per_Quad);

    Vec2 pen = {floor(baseline.x), floor(baseline.y)};
    u32 prev_codepoint = 0;
    for(size_t i = 0; i < text_len; i++){
        char c = text[i];

        // When to apply kerning based on sample code from here:
        // https://freetype.org/freetype2/docs/tutorial/step2.html#:~:text=c.%20Kerning
        Font_Glyph *glyph   = font_get_glyph(font, c);
        float kerning = font_get_kerning_advance(font, prev_codepoint, c);
        pen.x += kerning;

        Vec2 min_p = {pen.x + glyph->offset.x, pen.y + glyph->offset.y};
        Rect bounds = rect_from_min_wh(min_p, glyph->width, glyph->height);
        Rect uvs = rect_from_min_max(glyph->uv_min, glyph->uv_max);

        Draw_Vertex *vertex = &v[i*Draw__Vertex_Per_Quad];
        draw__set_quad(vertex, bounds, color, uvs);

        pen.x += (float)glyph->advance;
        prev_codepoint = c;
    }
}

void draw_set_camera(Camera *camera){
    Draw_Layer *layer = draw__get_active_layer();
    layer->camera = camera;
}

//
// Font
//

Font_Glyph* font_get_glyph(Font* font, u32 codepoint){
    Font_Glyph* result = &font->null_glyph;
    for(u32 i = 0; i < font->glyphs_count; i++){
        if(font->glyph_codepoints[i] == codepoint){
            result = &font->glyphs[i];
            break;
        }
    }
    return result;
}

float font_get_kerning_advance(Font* font, u32 prev_codepoint, u32 codepoint){
    float result = 0;
    for(u32 i = 0; i < font->kerning_pairs_count; i++){
        Font_Kerning *entry = &font->kerning_pairs[i];
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

bool font_load_from_memory(Font* font, const char* font_name, void *memory, size_t memory_size){
    Buffer buffer = {memory, memory_size};
    bool error = false;
    font->texture = Draw_Texture_Null;

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
                font->metrics = (Font_Metrics*)draw__read_bytes(&payload, sizeof(Font_Metrics), &error);
            } break;

            case Font_Section_Glyphs:{
                Font_Glyph *null_glyph = draw__read_bytes(&payload, sizeof(Font_Glyph), &error);
                u32 *count = (u32*)draw__read_bytes(&payload, sizeof(u32), &error);
                if(null_glyph && count){
                    font->null_glyph = *null_glyph;
                    font->glyphs_count = *count;
                    font->glyph_codepoints = (u32*)draw__read_bytes(
                        &payload, sizeof(u32) * font->glyphs_count, &error
                    );
                    font->glyphs = (Font_Glyph*)draw__read_bytes(
                        &payload, sizeof(Font_Glyph) * font->glyphs_count, &error
                    );
                }
            } break;

            case Font_Section_Kerning:{
                u32 *count = (u32*)draw__read_bytes(&payload, sizeof(u32), &error);
                if(count){
                    font->kerning_pairs = (Font_Kerning*)draw__read_bytes(&payload, sizeof(Font_Kerning), &error);
                    font->kerning_advance = (f32*)draw__read_bytes(&payload, sizeof(f32), &error);
                }
            } break;

            case Font_Section_Pixels:{
                u32 *w = (u32*)draw__read_bytes(&payload, sizeof(u32), &error);
                u32 *h = (u32*)draw__read_bytes(&payload, sizeof(u32), &error);
                if(w && h){
                    font->pixels_width  = *w;
                    font->pixels_height = *h;
                    u32 total_pixels = font->pixels_width*font->pixels_height;
                    font->pixels = (u32*)draw__read_bytes(&payload, sizeof(u32)*total_pixels, &error);

                    font->texture = draw_create_texture(*w, *h, font->pixels, 0);
                }
            } break;

            case Font_Section_Blank_UVs:
                continue;
        }

        if(error){
            fmt_msg("Error! Section type {1} for font {0} is malformd.\n", fmt_cstr(font_name), fmt_i(section->type));
        }
    }

    return !error;
}

#ifdef OS_Linux
//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------

#include "opengl.h"

#define Draw__Constants_Uniform_Binding 0

typedef struct{
    Mat4  mat_camera;
    Mat4  mat_model;
    Mat4  mat_light;
    Vec3  camera_pos;
    float time;
} Draw_Constants;

#define Shader_Preamble               \
"#version 330\n"                      \
"layout(std140) uniform Constants{\n" \
"    mat4  mat_camera;\n"             \
"    mat4  mat_model;\n"              \
"    mat4  mat_light;\n"              \
"    vec3  camera_pos;\n"             \
"    float time;\n"                   \
"};\n"

static const char *draw__2d_shader_vert_src =
Shader_Preamble
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

static const char *draw__2d_shader_frag_src =
Shader_Preamble
"in vec2  f_uv;\n"
"in vec4  f_color;\n"
"out vec4 out_color;\n"
"\n"
"uniform sampler2D u_texture;\n" // TODO: How do we set the texture index for the uniform? Is it one by default?
"\n"
"void main(){\n"
"    vec4 tex_color = texture(u_texture, f_uv);\n"
"    out_color = tex_color*vec4(f_color.rgb*f_color.a, f_color.a);\n"
"}\n";

static const char *draw__3d_shader_vert_src =
Shader_Preamble
"in vec3 v_pos;\n"
"in vec3 v_normal;\n"
"in vec2 v_uv;\n"
"in uint v_color;\n"
"\n"
"out vec2 f_uv;\n"
"out vec4 f_color;\n"
"\n"
"void main(){\n"
"    gl_Position = mat_camera*mat_model*vec4(v_pos, 1);\n"
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

static const char *draw__3d_shader_frag_src =
Shader_Preamble
"in vec2  f_uv;\n"
"in vec4  f_color;\n"
"out vec4 out_color;\n"
"\n"
"uniform sampler2D u_texture;\n" // TODO: How do we set the texture index for the uniform? Is it one by default?
"\n"
"void main(){\n"
"    out_color = vec4(f_color.rgb*f_color.a, f_color.a);\n"
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

bool draw_begin(Buffer *memory){
    Draw_State *s = &draw__state;

    Display_Info info = display_get_info();
    s->common.hw_rendering = info.window_flags & Display_Flag_HW_Rendering;
    s->common.memory = memory;

    bool success = true;
    if(s->common.hw_rendering){
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(draw__debug_msg_callback, NULL);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Using premultiplied alpha

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glDepthRange(-1000, 1000);

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

        s->common.shader_default_2d = draw__compile_shader(
            "Default 2D", draw__2d_shader_vert_src, draw__2d_shader_frag_src
        );
        s->common.shader_default_3d = draw__compile_shader(
            "Default 3D", draw__3d_shader_vert_src, draw__3d_shader_frag_src
        );

        /*s->default_shader = draw__compile_shader("Default 2D", draw__default_vert, draw__default_frag);*/

        u32 tex_w = 4;
        u32 tex_h = 4;
        u32 tex_pixels[tex_w*tex_h];
        for(u32 i = 0; i < tex_w*tex_h; i++){
            tex_pixels[i] = 0xffffffff;
        }
        s->common.blank_texture = draw_create_texture(tex_w, tex_h, &tex_pixels[0], 0);

        Draw_Layer *layers = &s->common.layers[0];
        for(u32 i = 0; i < Draw_Layer_Total; i++){
            layers[i].camera = &s->common.default_camera;
        }
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
        glClearColor(1, 1, 1, 1); // TODO: Allow configurable clear color. Also allow clearing on a per-layer basis.
        glClearDepth(1000);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        Draw_Constants constants = {};

        // Set the default camera. This is an orthographic camera set to the window size.
        // This would work best for 2D applications and HUD elements.
        Vec2 window_extents = v2_muls((Vec2){info.window_width, info.window_height}, 0.5f);
        Rect window_bounds  = {window_extents, window_extents};

        Camera *camera = &s->common.default_camera;
        camera->proj = orthographic_projection(window_bounds, -100, 100);
        camera->view.mat = Mat4_Identity;
        camera->view.inv = invert_view_matrix(Mat4_Identity);
        camera->center = (Vec3){0, 0, 0}; // TODO: Is this the correct center?
        camera->facing = (Vec3){0, 0, 1}; // TODO: Should z be -1?

        glBindBuffer(GL_UNIFORM_BUFFER, s->constants_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(constants), &constants);
    }
}

static void draw__shader_set_camera(Draw_State* s, Camera* camera){
    glBindBuffer(GL_UNIFORM_BUFFER, s->constants_ubo);
    Mat4 mat_camera = mat4_transpose(mat4_mul(camera->proj.mat, camera->view.mat));
    glBufferSubData(
        GL_UNIFORM_BUFFER, Offset_Of(Draw_Constants, mat_camera), sizeof(Mat4), &mat_camera
    );
}

void draw_frame_end(){
    Draw_State *s = &draw__state;
    Camera* camera = &s->common.default_camera;

    draw__shader_set_camera(s, camera);
    for(u32 layer_index = Draw_Layer_None+1; layer_index < Draw_Layer_Total; layer_index++){
        Draw_Layer *layer = &s->common.layers[layer_index];
        if(layer->camera != camera){
            assert(layer->camera);
            draw__shader_set_camera(s, layer->camera);
            camera = layer->camera;
        }

        GLuint current_shader = s->common.shader_default_2d;
        glUseProgram(current_shader);

        Draw_Cmd* cmd_base = layer->cmd_first;
        Draw_Texture texture = s->common.blank_texture;
        glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
        while(cmd_base){
            switch(cmd_base->type){
                default: assert(0);

                case Draw_Cmd_Type_Set_Shader:{
                    Draw_Cmd_Set_Shader *cmd = (Draw_Cmd_Set_Shader*)cmd_base;
                    current_shader = (GLuint)cmd->shader;
                    glUseProgram(current_shader);
                } break;

                case Draw_Cmd_Type_Quad:{
                    Draw_Cmd_Quad *cmd = (Draw_Cmd_Quad *)cmd_base;
                    assert(cmd->texture != Draw_Texture_Null);
                    if(texture != cmd->texture){
                        texture = cmd->texture;
                        glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
                    }

                    u32 total_vertex_size = cmd->vertex_count * sizeof(Draw_Vertex);
                    Draw_Vertex *v = (Draw_Vertex *)(cmd+1);

                    glBindBuffer(GL_ARRAY_BUFFER, s->quad_vbo);
                    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(total_vertex_size), v, GL_DYNAMIC_DRAW);
                    glDrawArrays(GL_TRIANGLES, 0, cmd->vertex_count);
                    /*glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->quad_index_buffer);*/
                    /*glDrawElements(GL_TRIANGLES, (GLsizei)(vertex_count * Draw__Indeces_Per_Quad), GL_UNSIGNED_INT, (GLvoid*)0);*/
                } break;

                case Draw_Cmd_Type_Vertices:{
                    Draw_Cmd_Vertices *cmd = (Draw_Cmd_Vertices*)cmd_base;

                    auto x_form = mat4_transpose(cmd->xform);
                    glBindBuffer(GL_UNIFORM_BUFFER, s->constants_ubo);
                    glBufferSubData(
                        GL_UNIFORM_BUFFER, Offset_Of(Draw_Constants, mat_model),
                        sizeof(Mat4), &x_form
                    );

                    glBindBuffer(GL_ARRAY_BUFFER, s->quad_vbo);
                    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(cmd->vertices_count*sizeof(Draw_Vertex)), cmd->vertices, GL_DYNAMIC_DRAW);
                    glDrawArrays(GL_TRIANGLES, 0, (u32)cmd->vertices_count);
                } break;
            }
            cmd_base = cmd_base->next;
        }

        layer->cmd_last  = NULL;
        layer->cmd_first = NULL;
    }
}

Draw_Texture draw_create_texture(u32 width, u32 height, u32 *pixels, u32 flags){
    assert(width > 0 && height > 0);
    GLint  internal_format = GL_RGBA8; // TODO: Do we care? Can we tell OpenGL we don't care?
    GLenum source_format   = GL_RGBA;

    GLuint handle = 0;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, source_format, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    /*if(flags & Texture_Flag_Wrap){*/
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    /*}*/
    /*else{*/
        /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);*/
        /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
    /*}*/

    Draw_Texture result = handle;
    return result;
}

void draw_destroy_texture(Draw_Texture *texture){
    GLuint t = (GLuint)*texture;
    glDeleteTextures(1, &t);
    *texture = Draw_Texture_Null;
}

//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------
#endif
