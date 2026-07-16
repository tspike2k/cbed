/*
Authors:   tspike (github.com/tspike2k)
Copyright: Copyright (c) 2026
License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
*/

#include "font.h"

Ceabed_API f32 font_get_kerning_advance(Font* font, u32 prev_codepoint, u32 codepoint){
    u8 *base = (u8 *)font;
    Font_Kerning_Pair *kerning_pairs = (Font_Kerning_Pair *)&base[font->kerning_pairs_offset];
    float *kerning_advance = (float *)&base[font->kerning_advance_offset];

    f32 result = 0.0f;
    for_count(u32, i, font->kerning_pairs_count){
        Font_Kerning_Pair *entry = &kerning_pairs[i];
        if(entry->a == prev_codepoint && entry->b == codepoint){
            result = kerning_advance[i];
            break;
        }
    }

    return result;
}

Ceabed_API Font_Glyph* font_get_glyph(Font* font, u32 codepoint){
    // IMPORTANT! Code that calls this function should test to ensure the font contains glyphs.
    assert(font->glyphs_count > 0);

    u8 *base           = (u8 *)font;
    u32 *codepoints    = (u32 *)&base[font->glyphs_codepoint_offset];
    Font_Glyph *glyphs = (Font_Glyph *)&base[font->glyphs_offset];

    // The first glyph is always the Null Glyph.
    Font_Glyph *result = &glyphs[0];

    for_count(u32, i, font->glyphs_count){
        if(codepoints[i] == codepoint){
            result = &glyphs[i];
            break;
        }
    }

    return result;
}

#ifdef FONT_BUILDER

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftstroke.h>
#include "img.h"

typedef struct{
    Buffer*      memory;
    Font_Info    font_info;

    u32          codepoints_count;
    u32         *codepoints;
    Font_Glyph  *glyphs;
    u32        **glyph_pixels;

    FT_Library lib;
    FT_Face    face;
    FT_Stroker stroker;
} Font_Builder;

static const char *font__get_font_path(Buffer* memory, const char *font_file_name){
    char *result = NULL;

    /*size_t memory_restore = memory_used;*/
    // TODO: Ideally, we should scan each directory listed in Font_Directories. That will require
    // us to write a directory scanner. In the meantime, we'll hardcode the path.
    const char* path = "/usr/share/fonts/TTF/";
    char* s = buffer_write_text(memory, path, strlen(path));
    buffer_write_text(memory, font_file_name, strlen(font_file_name));
    buffer_write_text(memory, "\0", 1);
    result = s;

    return result;
}

static bool font__builder_open(Font_Builder *s, const char *font_file_name, Buffer *memory){
    const char * font_path = font__get_font_path(memory, font_file_name);
    // TODO: Get error diagnostic from Freetype? Can you?

    s->memory = memory;
    if(FT_Init_FreeType(&s->lib) != 0){
        fmt_msg("Error! Failed to initialize Freetype2.\n");
        return false;
    }

    if(FT_New_Face(s->lib, font_path, 0, &s->face) != 0){
        fmt_msg("Error: Unable to load font file {0}. Aborting...\n", fmt_cstr(font_file_name));
        return false;
    }

    return true;
}

static void font__builder_close(Font_Builder *s){
    if(s->stroker) FT_Stroker_Done(s->stroker);
    if(s->face) FT_Done_Face(s->face);
    if(s->lib) FT_Done_FreeType(s->lib);
}

static u32 font__blend_colors_premultiplied_alpha(u32 source, u32 dest){
    // Blending code adapted from Handmade Hero.
    // TODO: Cite which day of Handmade Hero it was from

    // TODO: Gamma corrected colors? See here for more information:
    // https://www.youtube.com/watch?v=fVyzTKCfchw&feature=youtu.be&t=3275
    Vec4 s = {{
        (source >> 16) & 0xff, (source >>  8) & 0xff,
        (source >>  0) & 0xff, (source >> 24) & 0xff
    }};

    Vec4 d = {{
        (dest >> 16) & 0xff, (dest >>  8) & 0xff,
        (dest >>  0) & 0xff, (dest >> 24) & 0xff
    }};

    float rsa = (s.a / 255.0f);
    float rda = (d.a / 255.0f);
    float inv_rsa = (1.0f - rsa);
    Vec4 out_c = {{
        inv_rsa*d.r + s.r,
        inv_rsa*d.g + s.g,
        inv_rsa*d.b + s.b,
        (rsa + rda  - rsa * rda) * 255.0f
    }};
    u32 result = ((u32)out_c.a) << 24 | ((u32)out_c.r) << 16
                | ((u32)out_c.g) <<  8 | ((u32)out_c.b) <<  0;

    return result;
}

static void font__blit_glyph(FT_BitmapGlyph bitmap_glyph, u32 dest_w, u32 dest_h, u32* dest, u32 target_color, u32 offset_x, u32 offset_y){
    u32 w = bitmap_glyph->bitmap.width;
    u32 h = bitmap_glyph->bitmap.rows;

    for(u32 y = 0; y < h; y++){
        for(u32 x = 0; x < w; x++){
            u32 alpha = bitmap_glyph->bitmap.buffer[x + y*w];
            u32 color = premultiply_alpha((target_color & 0x00ffffff) | (alpha << 24));

            u32 pixel_index = offset_x+x + (offset_y+y)*dest_w;
            assert(pixel_index < dest_w*dest_h);
            u32 *pixel = &dest[pixel_index];
            *pixel = font__blend_colors_premultiplied_alpha(color, *pixel);
        }
    }
}

static FT_BitmapGlyph font__rasterize_glyph(FT_Face face, FT_Stroker stroker, u32 codepoint, u32 stroke){
    // TODO: Error handling!
    FT_Load_Char(face, codepoint, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
    FT_Glyph glyph_info;
    FT_Get_Glyph(face->glyph, &glyph_info);
    assert(glyph_info->format == FT_GLYPH_FORMAT_OUTLINE);

    if(stroke > 0)
        FT_Glyph_StrokeBorder(&glyph_info, stroker, false, true);

    FT_Glyph_To_Bitmap(&glyph_info, FT_RENDER_MODE_NORMAL, NULL, 1);
    FT_BitmapGlyph result = (FT_BitmapGlyph)glyph_info;
    return result;
}

static bool font__generate_glyph(Font_Builder *s, u32 codepoint, u32 index){
    // TODO: Better error handling!
    bool succeeded = true;

    FT_Face     face       = s->face;
    FT_Stroker  stroker    = s->stroker;
    Font_Info*  font_entry = &s->font_info;

    s->codepoints[index] = codepoint;
    Font_Glyph *glyph = &s->glyphs[index];

    if(!char_is_whitespace((char)codepoint)){
        FT_BitmapGlyph bitmap_glyph = font__rasterize_glyph(face, stroker, codepoint, font_entry->stroke);

        // Copy glyph metrics
        //
        // NOTE: The offset values are added to the pen position to correctly align the glyph bitmap
        // when rendering text. The x-offset is the left-side bearing of the glyph. The y-offset
        // expects glyph bitmaps to be drawn from the bottom-left, with the y-axis growing upwards.
        // The value of the y-offset is the descender and will be negative for glyphs that extend
        // below the baseline.
        //
        // FT_BitmapGlyph.left:        left-side bearing
        // FT_BitmapGlyph.top:         top-side bearing (ascender?)
        // FT_BitmapGlyph.bitmap.rows: glyph pixel height
        glyph->advance    = ((u32)face->glyph->advance.x) >> 6;
        glyph->offset.x  = bitmap_glyph->left;
        glyph->offset.y  = -((float)(bitmap_glyph->bitmap.rows) - (float)(bitmap_glyph->top)) + (float)font_entry->stroke; // NOTE: We must cast before negation as the metrics are unsigned integers

        // Copy the rasterized glyph
        glyph->width  = bitmap_glyph->bitmap.width;
        glyph->height = bitmap_glyph->bitmap.rows;
        s->glyph_pixels[index] = buffer_push_array(u32, s->memory, glyph->width*glyph->height);
        u32 *pixels = s->glyph_pixels[index];

        u32 target_color = font_entry->stroke == 0 ? font_entry->fill_color : font_entry->stroke_color;
        font__blit_glyph(bitmap_glyph, glyph->width, glyph->height, pixels, target_color, 0, 0);

        // If a stroke is defined, now draw the fill.
        if(font_entry->stroke){
            u32 stroke_left = bitmap_glyph->left;
            u32 stroke_top  = bitmap_glyph->top;

            bitmap_glyph = font__rasterize_glyph(face, stroker, codepoint, 0);
            u32 fill_offset_x = bitmap_glyph->left - stroke_left;
            u32 fill_offset_y = stroke_top - bitmap_glyph->top; // In Freetype the Y-axis of bitmaps grows upwards, hence the flipped subtraction.

            font__blit_glyph(
                bitmap_glyph, glyph->width, glyph->height, pixels,
                font_entry->fill_color, fill_offset_x, fill_offset_y
            );

            glyph->offset.x += fill_offset_x;
            //glyph.offset.y += fill_offset_y; // TODO: Should we account for stroke on the y-axis?
        }
    }
    else{
        FT_Load_Char(face, codepoint, FT_LOAD_DEFAULT);
        glyph->advance    = ((u32)face->glyph->advance.x) >> 6;
    }

    return succeeded;
}

Ceabed_API String font_generate(Font_Info info, const char* font_file_name, u32 *codepoints, u32 codepoints_count, Buffer *memory){
    assert(codepoints_count > 0);

    String result = {};
    Font_Builder builder = {};
    if(font__builder_open(&builder, font_file_name, memory)){
        FT_Set_Pixel_Sizes(builder.face, 0, info.height);
        if(info.stroke > 0){
            FT_Stroker_New(builder.lib, &builder.stroker);
            FT_Stroker_Set(builder.stroker, info.stroke*64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
        }

        // See here for a discussion on calculating the line gap:
        // https://freetype.nongnu.narkive.com/MyeGsd2a/ft-vert-advance-on-line-break
        // https://stackoverflow.com/a/30793586
        FT_Face face = builder.face;
        u32 internal_leading = (u32)((face->size->metrics.ascender - face->size->metrics.descender) >> 6) - face->size->metrics.y_ppem;
        u32 line_gap    = (u32)(face->size->metrics.height) >> 6;
        u32 cap_height  = (u32)((face->size->metrics.ascender >> 6) - internal_leading);
        u32 char_height = (u32)(face->bbox.yMax - face->bbox.yMin) >> 6;

        builder.font_info = info;
        builder.codepoints = codepoints;
        builder.glyphs = buffer_push_array(Font_Glyph, memory, codepoints_count);
        builder.glyph_pixels = buffer_push_array(u32*, memory, codepoints_count);

        // TODO: Rasterize the Null glyph here.
        u32 valid_codepoints_count = 0;
        for_count(u32, i, codepoints_count){
            if(font__generate_glyph(&builder, codepoints[i], valid_codepoints_count)){
                if(codepoints[i] == 'M'){
                    Buffer restore = *memory;
                    Font_Glyph *glyph = &builder.glyphs[i];
                    img_save_tga("test.tga", glyph->width, glyph->height, builder.glyph_pixels[i], memory);
                    *memory = restore;
                }

                valid_codepoints_count++;
            }
        }

        /*font__bake_glyphs_to_atlas(builder);*/

        // Begin writing the font to memory
        Buffer memory_before = *memory;
        Font *font = buffer_push_type(Font, memory);

        font->header.magic   = Font_File_Magic;
        font->header.version = Font_File_Version;
        font->line_gap    = line_gap;
        font->cap_height  = cap_height;
        font->char_height = char_height;
    }
    font__builder_close(&builder);

    return result;
}

#endif // FONT_BUILDER
