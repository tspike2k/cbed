//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_MATH_HPP
#define CEABED_MATH_HPP

#include "common.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern double pow(double x, double y);
extern double ceil(double x);
extern double sqrt(double x);
extern double floor(double x);

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

typedef struct{
    Vec2 center;
    Vec2 extents;
} Rect;

Rect rect_from_min_wh(Vec2 p, float w, float h);
Vec2 rect_min(Rect r);
Vec2 rect_max(Rect r);

typedef struct{
    float m[4][4];
} Mat4;

Mat4 mat4_mul(Mat4 a, Mat4 b);
Mat4 mat4_transpose(Mat4 a);

extern Mat4 Mat4_Identity;

u32 premultiply_alpha(u32 c);


#endif // CEABED_MATH_HPP
