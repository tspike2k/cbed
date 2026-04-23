//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_DRAW_H
#define CEABED_DRAW_H

// TODO: Rather than giving each layer a fixed-size buffer, perhaps we should go back to
// a linked-list approach. Calls to draw_text would allocate a command and a quad for every
// character in the string. This would complicate the internals, but simplify the API.
// Perhaps it's worth it?

#include "common.h"
#include "math.h"

// User configures the values between Draw_Layer_None and Draw_Layer_Total
enum{
    Draw_Layer_None,
    Draw_Layer_World,
    Draw_Layer_HUD,
    Draw_Layer_Total,
};

typedef size_t Draw_Texture;
typedef size_t Draw_Shader;

// Combining forward and inverse matrices into one struct thanks to Handmade Hero
typedef struct{
    Mat4 mat;
    Mat4 inv;
} Draw_XForm;

typedef struct {
    Draw_XForm proj;
    Draw_XForm view;
    Vec3       center;
    Vec3       facing;
} Camera;

typedef struct {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
    u32 color;
} Draw_Vertex;

Ceabed_API Camera *draw_get_default_camera();
Ceabed_API void draw_set_camera(Camera* camera);
Ceabed_API Vec2 camera_project(Camera* camera, Vec3 world_p, float screen_w, float screen_h);
Ceabed_API Vec3 camera_unproject(Camera* camera, Vec2 screen_p, float screen_w, float screen_h);

Ceabed_API Mat4 invert_view_matrix(Mat4 view);
Ceabed_API Mat4 make_lookat_matrix(Vec3 camera_pos, Vec3 look_pos, Vec3 up_pos);
Ceabed_API Draw_XForm orthographic_projection(Rect bounds, float n, float f);
Ceabed_API Draw_XForm camera_view_from_polar(Vec3 camera_polar, Vec3 camera_target, Vec3 up);

Ceabed_API bool draw_begin(Buffer *memory);
Ceabed_API void draw_end();
Ceabed_API void draw_frame_begin();
Ceabed_API void draw_frame_end();
Ceabed_API u32  draw_set_layer(u32 layer_index);

Ceabed_API void draw_rect(Rect r, u32 color);
Ceabed_API void draw_rect_textured(Rect r, uint32_t color, Draw_Texture texture, Rect uvs);
Ceabed_API void draw_rect_outline(Rect r, u32 color, float border);
Ceabed_API void draw_vertices(Mat4 xform, Draw_Vertex *v, size_t vertex_count);
Ceabed_API void draw_2d_line(Vec2 p0, Vec2 p1, u32 color, f32 thickness);
Ceabed_API void draw_circle(Vec2 center, float radius, u32 color);

// Default shaders
Ceabed_API void draw_set_shader_3D();
Ceabed_API void draw_set_shader_2D();

Ceabed_API Draw_Texture draw_create_texture(u32 width, u32 height, u32 *pixels, u32 flags);
Ceabed_API void draw_destroy_texture(Draw_Texture *texture);

//
// Fonts
//

typedef struct {
    uint32_t height;
    uint32_t line_gap;
    uint32_t cap_height;
    uint32_t char_height; // NOTE: Maximum character height
} Font_Metrics;

typedef struct{
    uint32_t width;
    uint32_t height;
    uint32_t advance;
    Vec2 offset;
    Vec2 uv_min;
    Vec2 uv_max;
} Font_Glyph;

typedef struct{
    uint32_t a;
    uint32_t b;
} Font_Kerning;

// TODO: I'm of two minds on how to handle fonts in the game. On the one hand, we could do like
// we've done in earlier projects. We load the font file into memory, extract and copy the data
// from the file into permanent memory. Another option is to load the entire file into memory
// for the duration of the program and have the Font type act as a "window" into that file's
// memory. Advantages to the second approach are faster loading times. Disadvantages are platform
// alignment requirements.

typedef struct{
    Font_Metrics  *metrics;
    uint32_t       glyphs_count;
    uint32_t      *glyph_codepoints;
    Font_Glyph    *glyphs;
    Font_Glyph     null_glyph;

    uint32_t      kerning_pairs_count;
    Font_Kerning *kerning_pairs;
    float        *kerning_advance;

    uint32_t  pixels_width;
    uint32_t  pixels_height;
    uint32_t *pixels;

    // "Blank" uvs allow the sprite batcher to draw solid colored quads without having to
    // switch textures.
    Vec2      blank_uv_min;
    Vec2      blank_uv_max;

    uint64_t texture;
} Font;

#define Font_File_Magic (('F' << 0) | ('o' << 8) | ('n' << 16) | ('t' << 24))
#define Font_File_Version 1

enum{
    Font_Section_None,
    Font_Section_Metrics,
    Font_Section_Glyphs,
    Font_Section_Kerning,
    Font_Section_Pixels,
    Font_Section_Blank_UVs
};

typedef struct {
    uint32_t magic;
    uint32_t version;
} Font_Header;

typedef struct {
    uint32_t type;
    uint32_t size;
} Font_Section;

typedef enum{
    Font_Align_Left,
    Font_Align_Center,
    Font_Align_Right,
} Font_Align;

Ceabed_API bool font_load_from_memory(Font* font, const char* font_name, void *memory, size_t memory_size);
Ceabed_API Font_Glyph* font_get_glyph(Font* font, uint32_t codepoint);
Ceabed_API float font_get_kerning_advance(Font* font, uint32_t prev_codepoint, uint32_t codepoint);
Ceabed_API void draw_text(Vec2 baseline, u32 color, Font *font, const char* text, size_t text_len);
Ceabed_API f32  font_get_text_width(Font* font, const char *text, size_t text_len);
Ceabed_API Vec2 font_align_text(Font *font, Font_Align font_align, const char *text, size_t text_len, Rect bounds);

#endif // CEABED_DRAW_H
