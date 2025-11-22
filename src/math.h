//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_MATH_HPP
#define CEABED_MATH_HPP

#define PI  3.141592653589793f
#define TAU (PI*2.0f)

typedef struct{
    float x, y;
} Vec2;

typedef struct{
    float x, y, z;
} Vec3;

typedef struct{
    float x, y, z, w;
} Vec4;

typedef struct{
    Vec2 center;
    Vec2 extents;
} Rect;

typedef struct{
    float m[4][4];
} Mat4;

Mat4 mat4_mul(Mat4 a, Mat4 b);
Mat4 mat4_transpose(Mat4 a);

extern Mat4 Mat4_Identity;

#endif // CEABED_MATH_HPP
