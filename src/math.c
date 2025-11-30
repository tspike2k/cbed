//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2024
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "math.h"

Rect rect_from_min_wh(Vec2 min_p, float w, float h){
    Vec2 extents = {w*0.5f, h*0.5f};
    Vec2 center  = {min_p.x + extents.x, min_p.y + extents.y};
    Rect result  = {center, extents};
    return result;
}

Rect rect_from_min_max(Vec2 min_p, Vec2 max_p){
    Vec2 extents = {(max_p.x - min_p.x)*0.5f, (max_p.y - min_p.y)*0.5f};
    Vec2 center  = {min_p.x + extents.x, min_p.y + extents.y};
    Rect result = {center, extents};
    return result;
}

Vec2 rect_min(Rect r){
    Vec2 result = {r.center.x - r.extents.x, r.center.y - r.extents.y};
    return result;
}

Vec2 rect_max(Rect r){
    Vec2 result = {r.center.x + r.extents.x, r.center.y + r.extents.y};
    return result;
}

Mat4 Mat4_Identity = {{
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},
}};

Mat4 mat4_mul(Mat4 a, Mat4 b){
    Mat4 result;

#define Mat4_OP(r, c) result.m[r][c] = a.m[r][0]*b.m[0][c] + a.m[r][1]*b.m[1][c] + a.m[r][2]*b.m[2][c] + a.m[r][3]*b.m[3][c]
    Mat4_OP(0, 0);
    Mat4_OP(0, 1);
    Mat4_OP(0, 2);
    Mat4_OP(0, 3);
    Mat4_OP(1, 0);
    Mat4_OP(1, 1);
    Mat4_OP(1, 2);
    Mat4_OP(1, 3);
    Mat4_OP(2, 0);
    Mat4_OP(2, 1);
    Mat4_OP(2, 2);
    Mat4_OP(2, 3);
    Mat4_OP(3, 0);
    Mat4_OP(3, 1);
    Mat4_OP(3, 2);
    Mat4_OP(3, 3);
#undef Mat4_OP

    return result;
}

Mat4 mat4_transpose(Mat4 a){
    Mat4 result;
#define Mat4_OP(r, c) result.m[r][c] = a.m[c][r];
    Mat4_OP(0, 0);
    Mat4_OP(0, 1);
    Mat4_OP(0, 2);
    Mat4_OP(0, 3);
    Mat4_OP(1, 0);
    Mat4_OP(1, 1);
    Mat4_OP(1, 2);
    Mat4_OP(1, 3);
    Mat4_OP(2, 0);
    Mat4_OP(2, 1);
    Mat4_OP(2, 2);
    Mat4_OP(2, 3);
    Mat4_OP(3, 0);
    Mat4_OP(3, 1);
    Mat4_OP(3, 2);
    Mat4_OP(3, 3);
#undef Mat4_OP

    return result;
}

u64 round_up_power_of_two(u64 n){
    // NOTE: Adapted from here:
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    n--;
    for(u32 byte_index = 0; byte_index < sizeof(u64); byte_index++){
        n |= n >> ((u64)pow(2, byte_index)); // ^^ is the pow operator in D
    }
    n++;
    return n;
}

u32 premultiply_alpha(u32 c){
    u8 r = (c) & 0xff;
    u8 g = (c >> 8) & 0xff;
    u8 b = (c >> 16) & 0xff;
    u8 a = (c >> 24) & 0xff;

    float fa = ((float)a / 255.0f);
    float fr = ((float)r / 255.0f) * fa;
    float fg = ((float)g / 255.0f) * fa;
    float fb = ((float)b / 255.0f) * fa;

    u32 result = (u32)(a << 24)
                | (u32)(fb * 255.0f + 0.5f) << 16
                | (u32)(fg * 255.0f + 0.5f) << 8
                | (u32)(fr * 255.0f + 0.5f);
    return result;
}

#if 0



f32 signf(f32 f){
    f32 result = f < 0.0f ? -1.0f : 1.0f;
    return result;
}

Vec2 vec2(float x, float y){
    Vec2 result = {{x, y}};
    return result;
}

Vec2 v2_rot(Vec2 v, float angle){
    Vec2 result = vec2(
        v.x * cosf(angle) - v.y * sinf(angle),
        v.x * sinf(angle) + v.y * cosf(angle)
    );
    return result;
}

Vec2 v2_add(Vec2 a, Vec2 b){
    Vec2 result = vec2(
        a.x + b.x,
        a.y + b.y
    );
    return result;
}

Vec2 v2_sub(Vec2 a, Vec2 b){
    Vec2 result = vec2(
        a.x - b.x,
        a.y - b.y
    );
    return result;
}

Vec2 v2_muls(Vec2 v, float s){
    Vec2 result = vec2(
        v.x * s,
        v.y * s
    );
    return result;
}

Vec2 v2_lerp(Vec2 a, Vec2 b, float t){
    Vec2 result = vec2(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t)
    );
    return result;
}

float v2_dot(Vec2 a, Vec2 b){
    float result = a.x*b.x + a.y*b.y;
    return result;
}

float v2_len_sq(Vec2 a){
    f32 result = v2_dot(a, a);
    return result;
}

float v2_length(Vec2 v){
    float result = sqrtf(v.x*v.x + v.y*v.y);
    return result;
}

float v2_dist_sq(Vec2 a, Vec2 b){
    f32 result = v2_len_sq(v2_sub(a, b));
    return result;
}

Vec4 v4_lerp(Vec4 a, Vec4 b, float t){
    Vec4 result = vec4(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
        lerp(a.w, b.w, t)
    );
    return result;
}

float lerp(float a, float b, float t){
    float result = (1.0f-t)*a + t*b;
    return result;
}

float squared(float a){
    f32 result = a*a;
    return result;
}

Vec2 v2_normalize(Vec2 v){
    Vec2 result = vec2(0, 0);
    float mag = sqrtf(v.x*v.x + v.y*v.y);
    if(mag > 0.00001f){
        result.x = v.x / mag;
        result.y = v.y / mag;
    }
    return result;
}

Vec3 vec3(float x, float y, float z){
    Vec3 result = {{x, y, z}};
    return result;
}

Vec4 vec4(float x, float y, float z, float w){
    Vec4 result = {{x, y, z, w}};
    return result;
}

Vec4 v2_to_v4(Vec2 v, float z, float w){
    Vec4 result = {{v.x, v.y, z, w}};
    return result;
}

Vec3 v2_to_v3(Vec2 v, float z){
    Vec3 result = {{v.x, v.y, z,}};
    return result;
}

Rect rect(Vec2 center, Vec2 extents){
    Rect result = {center, extents};
    return result;
}

Rect rect_from_min_max(Vec2 min, Vec2 max){
    Vec2 extents = vec2(0.5f*(max.x - min.x), 0.5f*(max.y - min.y));
    Vec2 center = v2_add(min, extents);
    Rect result = {center, extents};
    return result;
}

Rect rect_from_min_wh(Vec2 min, float w, float h){
    Vec2 extents = vec2(w*0.5f, h*0.5f);
    Rect result = rect(v2_add(min, extents), extents);
    return result;
}

f32 rect_left(Rect r){
    f32 result = r.center.x - r.extents.x;
    return result;
}

f32 rect_right(Rect r){
    f32 result = r.center.x + r.extents.x;
    return result;
}

f32 rect_top(Rect r){
    f32 result = r.center.y + r.extents.y;
    return result;
}

f32 rect_bottom(Rect r){
    f32 result = r.center.y - r.extents.y;
    return result;
}

f32 rect_width(Rect r){
    f32 result = r.extents.x*2.0f;
    return result;
}

f32 rect_height(Rect r){
    f32 result = r.extents.y*2.0f;
    return result;
}

bool rects_overlap(Rect a, Rect b){
    bool result = a.center.x - a.extents.x < b.center.x + b.extents.x
        && a.center.x + a.extents.x > b.center.x - b.extents.x
        && a.center.y - a.extents.y < b.center.y + b.extents.y
        && a.center.y + a.extents.y > b.center.y - b.extents.y;

    return result;
}

Vec2 rect_min(Rect r){
    Vec2 result = vec2(r.center.x - r.extents.x, r.center.y - r.extents.y);
    return result;
}

Vec2 rect_max(Rect r){
    Vec2 result = vec2(r.center.x + r.extents.x, r.center.y + r.extents.y);
    return result;
}

Rect rect_expand(Rect r, Vec2 size){
    Rect result = rect(r.center, v2_add(r.extents, size));
    return result;
}

Rect rect_cut_top(Rect* source, f32 height){
    f32 extents_y = height*0.5f;
    assert(source->extents.y >= extents_y);

    Rect result = rect(
        vec2(source->center.x, rect_top(*source) - extents_y),
        vec2(source->extents.x, extents_y)
    );

    source->center.y  -= extents_y;
    source->extents.y -= extents_y;
    return result;
}

Rect rect_cut_bottom(Rect* source, f32 height){
    f32 extents_y = height*0.5f;
    assert(source->extents.y >= extents_y);

    Rect result = rect(
        vec2(source->center.x, rect_bottom(*source) + extents_y),
        vec2(source->extents.x, extents_y)
    );

    source->center.y  += extents_y;
    source->extents.y -= extents_y;
    return result;
}

bool point_inside_rect(Vec2 p, Rect r){
    Vec2 rel = v2_sub(r.center, p);
    bool result = fabsf(rel.x) <= r.extents.x && fabsf(rel.y) <= r.extents.y;
    return result;
}

bool ray_vs_rect(Vec2 start, Vec2 delta, Rect bounds, float* t_min, Vec2* hit_normal)
{
    // Adapted from here:
    // https://noonat.github.io/intersect
    f32 scaleX = 1.0f / delta.x;
    f32 scaleY = 1.0f / delta.y;
    f32 signX = sign_f32(scaleX);
    f32 signY = sign_f32(scaleY);

    f32 nearTimeX = (bounds.center.x - signX*bounds.extents.x - start.x) * scaleX;
    f32 nearTimeY = (bounds.center.y - signY*bounds.extents.y - start.y) * scaleY;
    f32 farTimeX  = (bounds.center.x + signX*bounds.extents.x - start.x) * scaleX;
    f32 farTimeY  = (bounds.center.y + signY*bounds.extents.y - start.y) * scaleY;

    if(nearTimeX > farTimeY || nearTimeY > farTimeX){
        return false;
    }

    f32 nearTime = max_f32(nearTimeX, nearTimeY);
    f32 farTime  = min_f32(farTimeX, farTimeY);
    if(nearTime >= 1.0f || farTime <= 0.0f){
        return false;
    }

    // If nearTime <= 0.0f, then the origin of the ray is within the rect.
    // Should we signal this somehow and let the collision handling code
    // resolve this?
    if(nearTime > 0.0f && nearTime < *t_min){
        *t_min = max_f32(nearTime - Swept_Test_Epsilon, 0.0f); // TODO: Better epsilon
        if(nearTimeX > nearTimeY)
            *hit_normal = vec2(-signX, 0);
        else
            *hit_normal = vec2(0, -signY);

        return true;
    }

    return false;
}

f32 min_f32(f32 a, f32 b){
    f32 result = a < b ? a : b;
    return result;
}

f32 max_f32(f32 a, f32 b){
    f32 result = a > b ? a : b;
    return result;
}

f32 clamp_f32(f32 a, f32 min, f32 max){
    f32 result = a;
    if(a < min){
        result = min;
    }
    else if(a > max){
        result = max;
    }
    return result;
}

uint32_t min_u32(uint32_t a, uint32_t b){
    uint32_t result = a < b ? a : b;
    return result;
}

uint32_t max_u32(uint32_t a, uint32_t b){
    uint32_t result = a > b ? a : b;
    return result;
}

size_t min_size_t(size_t a, size_t b){
    size_t result = a < b ? a : b;
    return result;
}

f32 sign_f32(f32 a){
    f32 result = a >= 0.0f ? 1.0f : -1.0f;
    return result;
}

void decrement_and_wrap_u32(u32 *n, u32 max){
    *n = (*n + max - 1) % max;
}

void increment_and_wrap_u32(u32 *n, u32 max){
    *n = (*n + 1) % max;
}

u32 premultiply_alpha(u32 source_color){
    u32 result = 0;

    u8 a = cast(u8)(source_color >> 24);
    if(a > 0){
        u8 r = cast(u8)(source_color >>  0);
        u8 g = cast(u8)(source_color >>  8);
        u8 b = cast(u8)(source_color >> 16);

        // TODO: Gamma corrected colors? See here for more information:
        // https://www.youtube.com/watch?v=fVyzTKCfchw&feature=youtu.be&t=3275
        float fa =  cast(f32)a / 255.0f;
        float fr = (cast(f32)r / 255.0f) * fa;
        float fg = (cast(f32)g / 255.0f) * fa;
        float fb = (cast(f32)b / 255.0f) * fa;

        result =
            a << 24
            | cast(u32)(fb * 255.0f + 0.5f) << 16
            | cast(u32)(fg * 255.0f + 0.5f) << 8
            | cast(u32)(fr * 255.0f + 0.5f) << 0
        ;
    }
    return result;
}

f32 normalized_range_clamp(f32 a, f32 min, f32 max){
    // Handy function thanks to Casey Muratori, Handmade Hero day 107.
    a = clamp_f32(a, min, max);
    f32 result = (a - min) / (max - min);
    return result;
}
#endif
