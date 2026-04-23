//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_MATH_H
#define CEABED_MATH_H

#include "common.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))

Ceabed_C_Lib double pow(double x, double y);
Ceabed_C_Lib double ceil(double x);
Ceabed_C_Lib double sqrt(double x);
Ceabed_C_Lib double floor(double x);
Ceabed_C_Lib double sin(double x);
Ceabed_C_Lib double cos(double x);

#define PI  3.141592653589793f
#define TAU (PI*2.0f)

typedef struct{
    float x, y;
} Vec2;

typedef struct{
    float x, y, z;
} Vec3;

typedef union{
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };
} Vec4;

Ceabed_API Vec2  v2(f32 x, f32 y);
Ceabed_API Vec2  v2_sub(Vec2 a, Vec2 b);
Ceabed_API Vec2  v2_add(Vec2 a, Vec2 b);
Ceabed_API Vec2  v2_mul(Vec2 a, Vec2 b);
Ceabed_API Vec2  v2_muls(Vec2 a, float b);
Ceabed_API Vec2  v2_div(Vec2 a, Vec2 b);
Ceabed_API Vec3  v3_sub(Vec3 a, Vec3 b);
Ceabed_API Vec3  v3_add(Vec3 a, Vec3 b);
Ceabed_API Vec3  v3_mul(Vec3 a, Vec3 b);
Ceabed_API Vec3  v3_muls(Vec3 a, float b);
Ceabed_API Vec3  v3_div(Vec3 a, Vec3 b);
Ceabed_API Vec2  v2_normalize(Vec2 v);
Ceabed_API Vec3  v3_normalize(Vec3 a);
Ceabed_API float v2_squared(Vec2 v);
Ceabed_API float v3_squared(Vec3 v);
Ceabed_API float v2_length(Vec2 v);
Ceabed_API float v3_length(Vec3 v);
Ceabed_API float v2_dot(Vec2 a, Vec2 b);
Ceabed_API float v3_dot(Vec3 a, Vec3 b);
Ceabed_API Vec3  cross(Vec3 a, Vec3 b);
Ceabed_API Vec4  v4(f32 x, f32 y, f32 z, f32 w);

Ceabed_API float squared(float n);
Ceabed_API float v2_dist_sq(Vec2 a, Vec2 b);
Ceabed_API float v3_dist_sq(Vec3 a, Vec3 b);

typedef struct{
    Vec2 center;
    Vec2 extents;
} Rect;

Ceabed_API Rect rect_from_min_wh(Vec2 p, float w, float h);
Ceabed_API Rect rect_from_min_max(Vec2 min_p, Vec2 max_p);
Ceabed_API Vec2 rect_min(Rect r);
Ceabed_API Vec2 rect_max(Rect r);
Ceabed_API f32  rect_left(Rect r);
Ceabed_API f32  rect_right(Rect r);
Ceabed_API f32  rect_top(Rect r);
Ceabed_API f32  rect_bottom(Rect r);
Ceabed_API f32  rect_width(Rect r);
Ceabed_API f32  rect_height(Rect r);
Ceabed_API bool is_point_inside_rect(Vec2 p, Rect r);
Ceabed_API bool rects_overlap(Rect a, Rect b);

Ceabed_API Rect rect_shrink(Rect a, Vec2 size);
Ceabed_API Rect rect_expand(Rect a, Vec2 size);
// rect_cut_X concept thanks to Jonathan Blow
Rect rect_cut_left(Rect r, f32 size);
Rect rect_cut_right(Rect r, f32 size);
Rect rect_cut_top(Rect r, f32 size);
Rect rect_cut_bottom(Rect r, f32 size);

typedef struct{
    float m[4][4];
} Mat4;

Ceabed_API Mat4 mat4_mul(Mat4 a, Mat4 b);
Ceabed_API Mat4 mat4_transpose(Mat4 a);
Ceabed_API Mat4 mat4_translate(Vec3 offset);

#define Mat4_Identity ((Mat4){{ \
    {1, 0, 0, 0},               \
    {0, 1, 0, 0},               \
    {0, 0, 1, 0},               \
    {0, 0, 0, 1},               \
}})

Ceabed_API u32 premultiply_alpha(u32 c);
Ceabed_API float deg_to_rad(float degrees);
Ceabed_API Vec3 polar_to_world(Vec3 polar, Vec3 target_pos);

#ifdef __cplusplus

template<typename T> T min(T a, T b){
    T result = MIN(a, b);
    return result;
}

template<typename T> T max(T a, T b){
    T result = MAX(a, b);
    return result;
}

template<typename T> void clamp(T* a, T min_val, T max_val){
    *a = CLAMP(*a, min_val, max_val);
}

Vec2 operator+(Vec2 l, Vec2 r);

#endif // __cplusplus

#endif // CEABED_MATH_HPP
