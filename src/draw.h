//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_DRAW_H
#define CEABED_DRAW_H

#include <stdint.h>
#include <stddef.h>
#include "math.h"

typedef size_t Draw_Texture;

// User configures the values between Draw_Layer_None and Draw_Layer_Total
enum{
    Draw_Layer_None,
    Draw_Layer_World,
    Draw_Layer_Total,
};

typedef struct{
    Mat4 mat;
    Mat4 inv;
} Mat4_Pair;

Mat4_Pair orthographic_projection(Rect bounds, float n, float f);

bool draw_begin(void *memory, size_t memory_count);
void draw_end();
void draw_frame_begin();
void draw_frame_end();
void draw_quad(float px, float py, float w, float h, uint32_t color);

#endif // CEABED_DRAW_H
