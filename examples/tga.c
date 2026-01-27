//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

//
// A simple example program that shows how to write to TGA files.
//

#include "common.c"
#include "img.c"
#include "files.c"

#define Pixels_W 2
#define Pixels_H 2

u8 g_memory[4096];
u8 g_scratch[4096];
u32 g_pixels[Pixels_W*Pixels_H];

int main(){
    ceabed_begin();

    Buffer memory = {&g_memory[0], Array_Len(g_memory)};
    Buffer scratch = {&g_scratch[0], Array_Len(g_scratch)};

    g_pixels[0 + 0 * Pixels_W] = 0xff0000ff;
    g_pixels[1 + 0 * Pixels_W] = 0x00ff00ff;
    g_pixels[0 + 1 * Pixels_W] = 0x0000ffff;
    g_pixels[1 + 1 * Pixels_W] = 0xffffffff;

    img_save_tga("test.tga", Pixels_W, Pixels_H, &g_pixels[0], &scratch);
    Img_Pixels pixels = img_load_tga_from_file("test.tga", &memory, &scratch);
    img_save_tga("test2.tga", pixels.width, pixels.height, pixels.data, &scratch);

    ceabed_end();
    return 0;
}
