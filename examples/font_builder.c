/*
Authors:   tspike (github.com/tspike2k)
Copyright: Copyright (c) 2026
License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
*/

#define FONT_BUILDER

#include "common.c"
#include "files.c"
#include "img.c"
#include "font.c"
#include "math.c"

u8 g_memory[32*1024*1204];

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    Font_Info font_info = {
        .height       = 18,
        .stroke       = 1,
        .fill_color   = 0xffffffff,
        .stroke_color = 0xffff0000
    };

    char min_char = '!';
    char max_char = '~';
    uint codepoints[max_char - min_char];
    for_count(u32, i, max_char - min_char){
        codepoints[i] = ((char)min_char + i);
    }

    Font_Builder *builder = font_builder_begin(&memory);

    String dest = font_builder_generate(builder, font_info, "DejaVuSerif.ttf", &codepoints[0], Array_Len(codepoints));
    if(dest.size){
        file_write_from_memory("./bin/font2.fnt", dest.text, dest.size);
    }

    font_builder_end(builder);
    return 0;
}
