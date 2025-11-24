//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_IMG
#define CEABED_IMG

#include "common.h"

typedef struct{
    u32 *pixels;
    u32 width;
    u32 height;
} Img_Pixels;

Img_Pixels img_load_tga(const char* file_name);
bool       img_save_tga(Buffer *dest, const char* file_name, u32 width, u32 height, u32 *pixels);

#endif // CEABED_IMG
