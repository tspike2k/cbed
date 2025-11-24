//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "img.h"
#include <assert.h>

////
//
// TGA Files
//
////

/*
TGA loading code based on text from the following source:
https://paulbourke.net/dataformats/tga/
*/

#define TGA_Data_Type_Uncompressed_RGB 2
#define TGA_Desc_Upper_Left_Origin     (1 << 5)

#pragma pack(push, 1)
typedef struct {
    u8 id_length;
    u8 colormap_type;
    u8 data_type;
    s16 colormap_origin;
    s16 colormap_length;
    u8 colormap_depth;
    s16 x_origin;
    s16 y_origin;
    s16 width;  // TODO: Is this really signed?
    s16 height; // TODO: Is this really signed?
    u8 bits_per_pixel;
    u8 image_desc;
} TGA_Header;
#pragma pack(pop)

static bool img__tga_is_non_interleaved(TGA_Header* header){
    auto result = !((header->image_desc) & ((1 << 6) | (1 << 7)));
    return result;
}

// TODO: Implement this!
Img_Pixels img_load_tga(const char* file_name){
    Img_Pixels result = {};
    assert(0);
    return result;
}

bool img_save_tga(Buffer *dest, const char* file_name, u32 width, u32 height, u32 *pixels){
    size_t restore = dest->used;

    TGA_Header *header     = buffer_push_type(TGA_Header, dest);
    header->data_type      = TGA_Data_Type_Uncompressed_RGB;
    header->bits_per_pixel = 32;
    header->width          = (s16)width;
    header->height         = (s16)height;
    header->image_desc     = TGA_Desc_Upper_Left_Origin;

        u32 *out_pixels = buffer_push_array(u32, dest, width*height);
        for(u32 i = 0; i < width*height; i++){
            auto pixel = &pixels[i];

            u8 a = (*pixel >> 24) & 0xff;
            u8 r = (*pixel >> 16) & 0xff;
            u8 g = (*pixel >> 8)  & 0xff;
            u8 b = (*pixel)       & 0xff;

            out_pixels[i] = ((u32)a) << 24 | ((u32)b) << 16
                      |      ((u32)g) << 8 | ((u32)r);
        }

    bool result = true; // TODO: Do we have a way to tell if a write succeeds?
    file_write_from_memory(file_name, &dest->data[restore], dest->used - restore);
    return result;

    dest->used = restore;
    return result;
}


#if 0

Pixels load_tga_file(String file_name, Allocator *allocator){
    push_frame(allocator.scratch);
    scope(exit) pop_frame(allocator.scratch);

    auto file_contents = read_file_into_memory(file_name, allocator.scratch);
    auto reader = Serializer(file_contents);

    auto header = zero_type!TGA_Header;
    read(&reader, header);

    Pixels result;
    // TODO: Handle 24-bit pixel data
    if(header.data_type != TGA_Data_Type_Uncompressed_RGB){
        log_error("Unsupported TGA file for {0}. File must be an uncompressed RGB image.\n", file_name);
        return result;
    }

    if(header.bits_per_pixel != 32){
        log_error("Unsupported TGA file for {0}. File must be a 32-bit image (got {1}-bit).\n", file_name, header.bits_per_pixel);
        return result;
    }

    if(!(header.image_desc & TGA_Desc_Upper_Left_Origin)){
        log_error("Unsupported TGA file for {0}. Origin must be at the upper-left.\n", file_name);
        return result;
    }

    if(!is_non_interleaved(&header)){
        log_error("Unsupported TGA file for {0}. Pixel data must be non-interleaved.\n", file_name);
        return result;
    }

    uint width  = header.width;
    uint height = header.height;
    auto pixels = eat_array!uint(&reader, width*height);
    if(!pixels.length){
        log_error("Unable to read pixel data from TGA file {0}\n", file_name);
        return result;
    }

    foreach(ref pixel; pixels){
        u8 a = (pixel >> 24) & 0xff;
        u8 r = (pixel >> 16) & 0xff;
        u8 g = (pixel >> 8)  & 0xff;
        u8 b = (pixel)       & 0xff;

        pixel = (cast(uint)r)       | (cast(uint)g) << 8
              | (cast(uint)b) << 16 | (cast(uint)a) << 24;
    }

    result.width  = width;
    result.height = height;
    result.data   = dup_array(pixels, allocator);

    return result;
}
#endif
