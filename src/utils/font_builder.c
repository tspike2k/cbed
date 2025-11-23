/*
Copyright (c) 2025 tspike2k@github.com
Distributed under the Boost Software License, Version 1.0.
See accompanying file LICENSE_BOOST.txt or copy at http://www.boost.org/LICENSE_1_0.txt
*/

#include <stdint.h>
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftstroke.h>
#include "draw.h"
#include "common.c"
#include "files.c"
#include "math.c"
#include "atlas.c"

extern size_t strlen(const char *s);

#define Atlas_Padding 1

typedef struct{
    u32 height;
    u32 stroke;
    u32 fill_color;
    u32 stroke_color;
    const char *source_file_name;
    const char *dest_file_name;
} Font_Entry;

typedef struct {
    u32    codepoint;
    u32    pixels_width;
    u32    pixels_height;
    u32   *pixels;
    Font_Glyph  glyph;
} Rasterized_Glyph;

typedef struct{
    Buffer        memory;
    Font_Entry*   font_entry;
    Atlas_Packer  atlas;

    Font_Metrics metrics;

    FT_Library lib;
    FT_Face    face;
    FT_Stroker stroker;
    u32        fill_color;
    u32        stroke_color;
} Font_Builder;

#ifdef OS_Linux
static const char* Font_Directories[] = {
    "/usr/share/fonts/",
    "./"
};
#endif

static Font_Entry Font_Entries[] = {
    {
        .height=18, .stroke=0,
        .fill_color=0xffffffff, .stroke_color= 0x000000ff,
        .dest_file_name="./font.fnt",
        .source_file_name="DejaVuSerif.ttf"
    },
};

static bool font_builder_begin(Font_Builder* builder){
    bool result = true;
    if(FT_Init_FreeType(&builder->lib) != 0){
        result = false;
        // TODO: Get error diagnostic from Freetype? Can you?
        fmt_msg("ERROR! Failed to initialize Freetype2.\n");
    }
    return result;
}

static void font_builder_end(Font_Builder* builder){
    assert(builder->lib);
    FT_Done_FreeType(builder->lib);
}

static uint8_t g_memory[16*1024*1024];

static void push_null_char(Buffer *buffer){
    char *result = (char*)buffer_push_bytes(buffer, 1);
    if(result){
        *result = '\0';
    }
}

static const char *get_path_for_ttf_file(Buffer* memory, const char *file_name){
    char *result = NULL;

    /*size_t memory_restore = memory_used;*/
    // TODO: Ideally, we should scan each directory listed in Font_Directories. That will require
    // us to write a directory scanner. In the meantime, we'll hardcode the path.
    const char* path = "/usr/share/fonts/TTF/";
    char* s = buffer_write_text(memory, path, strlen(path));
    buffer_write_text(memory, file_name, strlen(file_name));
    push_null_char(memory);
    result = s;

    return result;
}

static u32 blend_colors_premultiplied_alpha(u32 source, u32 dest){
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

static FT_BitmapGlyph make_bitmap_glyph(FT_Face face, FT_Stroker stroker, u32 codepoint, u32 stroke){
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

static void blit_to_dest(FT_BitmapGlyph bitmap_glyph, Rasterized_Glyph* dest, u32 target_color, u32 offset_x, u32 offset_y){
    u32 w = bitmap_glyph->bitmap.width;
    u32 h = bitmap_glyph->bitmap.rows;

    for(u32 y = 0; y < h; y++){
        for(u32 x = 0; x < w; x++){
            u32 alpha = bitmap_glyph->bitmap.buffer[x + y*w];
            u32 color = premultiply_alpha((target_color & 0x00ffffff) | (alpha << 24)); // TODO: Wait, doesn't this literally do nothing?

            u32 pixel_index = offset_x+x + (offset_y+y)*dest->pixels_width;
            assert(pixel_index < dest->pixels_width*dest->pixels_height);
            u32 *pixel = &dest->pixels[pixel_index];
            *pixel = blend_colors_premultiplied_alpha(color, *pixel);
        }
    }
}

static bool rasterize_glyph_and_copy_metrics(Font_Builder *builder, u32 codepoint, Rasterized_Glyph *dest){
    // TODO: Better error handling!
    bool succeeded = true;

    FT_Face     face       = builder->face;
    FT_Stroker  stroker    = builder->stroker;
    Font_Entry* font_entry = builder->font_entry;

    FT_BitmapGlyph bitmap_glyph = make_bitmap_glyph(face, stroker, codepoint, font_entry->stroke);
    dest->codepoint     = codepoint;
    dest->pixels_width  = bitmap_glyph->bitmap.width;
    dest->pixels_height = bitmap_glyph->bitmap.rows;
    dest->pixels        = (u32*)buffer_push_bytes(&builder->memory, sizeof(u32)*dest->pixels_width*dest->pixels_height);

    // Copy glyph metrics
    Font_Glyph *glyph = &dest->glyph;
    glyph->width      = dest->pixels_width;
    glyph->height     = dest->pixels_height;
    glyph->advance    = ((u32)face->glyph->advance.x) >> 6;

    // NOTE: The offset values are added to the pen position to correctly align the glyph bitmap
    // when rendering text. The x-offset is the left-side bearing of the glyph. The y-offset
    // expects glyph bitmaps to be drawn from the bottom-left, with the y-axis growing upwards.
    // The value of the y-offset is the descender and will be negative for glyphs that extend
    // below the baseline.
    //
    // FT_BitmapGlyph.left:        left-side bearing
    // FT_BitmapGlyph.top:         top-side bearing (ascender?)
    // FT_BitmapGlyph.bitmap.rows: glyph pixel height
    glyph->offset.x  = bitmap_glyph->left;
    // NOTE: We must cast before negation as the metrics are unsigned integers
    glyph->offset.y  = -((float)(bitmap_glyph->bitmap.rows) - (float)(bitmap_glyph->top)) + (float)font_entry->stroke;

    u32 target_color = font_entry->stroke == 0 ? builder->fill_color : builder->stroke_color;
    blit_to_dest(bitmap_glyph, dest, target_color, 0, 0);

    if(font_entry->stroke){
        u32 stroke_left = bitmap_glyph->left;
        u32 stroke_top  = bitmap_glyph->top;

        bitmap_glyph = make_bitmap_glyph(face, stroker, codepoint, 0);
        u32 fill_offset_x = bitmap_glyph->left - stroke_left;
        u32 fill_offset_y = stroke_top - bitmap_glyph->top; // In Freetype the Y-axis of bitmaps grows upwards, hence the flipped subtraction.
        blit_to_dest(bitmap_glyph, dest, builder->fill_color, fill_offset_x, fill_offset_y);

        glyph->offset.x += fill_offset_x;
        //glyph.offset.y += fill_offset_y; // TODO: Should we account for stroke on the y-axis?
    }

    return succeeded;
}

static void add_codepoint(Font_Builder* builder, u32 codepoint){
    auto glyph = buffer_push_type(Rasterized_Glyph, &builder->memory);
    if(rasterize_glyph_and_copy_metrics(builder, codepoint, glyph)){
        atlas_packer_add(&builder->atlas, glyph->pixels_width, glyph->pixels_height, glyph);
    }
    else{
        fmt_msg("Unable to add codepoint {0} to font file {1}\n", fmt_i(codepoint), fmt_cstr(builder->font_entry->dest_file_name));
    }
}

static bool begin_building_font(Font_Builder *builder, const char *source_file_name, Font_Entry *entry){
    builder->font_entry   = entry;
    builder->fill_color   = entry->fill_color;
    builder->stroke_color = entry->stroke_color;

    if(FT_New_Face(builder->lib, source_file_name, 0, &builder->face) != 0){
        fmt_msg("Unable to load font file {0}. Aborting...\n", fmt_cstr(source_file_name));
        return false;
    }

    FT_Set_Pixel_Sizes(builder->face, 0, entry->height);
    if(entry->stroke > 0){
        FT_Stroker_New(builder->lib, &builder->stroker);
        FT_Stroker_Set(builder->stroker, entry->stroke*64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
    }

    // See here for a discussion on calculating the line gap:
    // https://freetype.nongnu.narkive.com/MyeGsd2a/ft-vert-advance-on-line-break
    // https://stackoverflow.com/a/30793586
    Font_Metrics *metrics = &builder->metrics;
    FT_Face face = builder->face;
    u32 internal_leading = (u32)((face->size->metrics.ascender - face->size->metrics.descender) >> 6) - face->size->metrics.y_ppem;
    metrics->height      = entry->height; // TODO: Is it safe to assume the font height given by Freetype2 will match our request?
    metrics->line_gap    = (u32)(face->size->metrics.height) >> 6;
    metrics->cap_height  = (u32)((face->size->metrics.ascender >> 6) - internal_leading);
    metrics->char_height = (u32)(face->bbox.yMax - face->bbox.yMin) >> 6;

    builder->atlas = atlas_packer_begin(&builder->memory);
    return true;
}

#define write_type_to_section(section, buffer, t) write_to_section(section, buffer, t, sizeof(*t))
static void write_to_section(Font_Section *section, Buffer *dest, void *data, size_t data_size){
    section->size += data_size;
    buffer_write(dest, data, data_size);
}

static Font_Section *push_section(Buffer *buffer, u32 section_type){
    Font_Section *result = buffer_push_type(Font_Section, buffer);
    result->type = section_type;
    result->size = sizeof(Font_Section);
    return result;
}

void end_building_font(Font_Builder* builder, Font_Entry *font_entry){
    Atlas_Packer *atlas = &builder->atlas;
    atlas_packer_end(atlas, Atlas_Padding, true);

    // Write the destination file in-place.
    size_t dest_start = builder->memory.used;

    Font_Header header = {};
    header.magic   = Font_File_Magic;
    header.version = Font_File_Version;

    Buffer *dest = &builder->memory;
    buffer_write_type(dest, &header);

    Font_Section *section = push_section(dest, Font_Section_Glyphs);
    u32 glyphs_count = atlas->items_count;
    write_type_to_section(section, dest, &glyphs_count);

    file_write_from_memory(font_entry->dest_file_name, &dest->data[dest_start], dest->used - dest_start);

/*
    u32 canvas_width   = atlas->canvas_width;
    u32 canvas_height  = atlas->canvas_height;
    u32 *canvas_pixels = (u32*)buffer_push_bytes(&builder->memory, sizeof(u32)*canvas_width*canvas_height);

    Atlas_Node *node = atlas->items;
    // Copy all rasterized glyphs into the sprite atlas.
    while(node){
        Rasterized_Glyph *glyph = (Rasterized_Glyph*)node->source;
        Font_Glyph *glyph_info  = &glyph->glyph;

        u32 source_width   = glyph->pixels_width;
        u32 source_height  = glyph->pixels_height;
        u32 *source_pixels = glyph->pixels;

        u32 dest_x = node->x;
        u32 dest_y = node->y;
        u32 w = node->width;
        u32 h = node->height;

        for(u32 y = 0; y < h; y++){
            for(u32 x = 0; x < w; x++){
                u32 canvas_i = dest_x + x + (dest_y+y) * canvas_width;
                assert(canvas_i < canvas_width*canvas_height);
                u32 source_i = x + y*w;
                assert(source_i < source_width*source_height);
                canvas_pixels[canvas_i] = source_pixels[source_i];
            }
        }

        glyph_info->uv_min = (Vec2){
            ((float)dest_x) / ((float)canvas_width),
            ((float)dest_y) / ((float)canvas_height)
        };

        glyph_info->uv_max = (Vec2){
            ((float)(dest_x + source_width))  / ((float)canvas_width),
            ((float)(dest_y + source_height)) / ((float)canvas_height)
        };

        node = node->next;
    }
*/

#if 0
    // Build the kerning table
    u32 kerning_count           = atlas->items_count*atlas->items_count;
    Font_Kerning *kerning_pairs = buffer_push_array(Font_Kerning, &builder->memory, kerning_count);
    float *     kerning_advance = buffer_push_array(float, &builder->memory, kerning_count);

    auto face = builder->face;

    u32 kerning_index = 0;
    auto item_a = atlas.items;
    while(item_a){
        auto item_b = atlas.items;
        auto glyph_a = cast(Rasterized_Glyph*)item_a.source;
        while(item_b){
            auto glyph_b = cast(Rasterized_Glyph*)item_b.source;

            auto codepoint_a = glyph_a.glyph.codepoint;
            auto codepoint_b = glyph_b.glyph.codepoint;

            auto glyph_index_a = FT_Get_Char_Index(face, codepoint_a);
            auto glyph_index_b = FT_Get_Char_Index(face, codepoint_b);

            FT_Vector kerning = FT_Vector(0, 0);
            FT_Get_Kerning(builder.face, glyph_index_a, glyph_index_b, FT_KERNING_DEFAULT, &kerning);

            kerning_pairs[kerning_index]   = Kerning_Pair(codepoint_a, codepoint_b);
            kerning_advance[kerning_index] = (kerning.x >> 6);
            kerning_index++;

            item_b = item_b.next;
        }

        item_a = item_a.next;
    }
    assert(kerning_index == kerning_count);

    auto trimmed_file_name = trim_file_extension(trim_leading_path(font_entry.dest_file_name));
    auto dest_tga_file_name = gen_string("{0}.tga", trimmed_file_name, allocator);
    save_tga_file(dest_tga_file_name, canvas_data.ptr, canvas_width, canvas_height, allocator);

    auto dest_memory = begin_reserve_all(allocator);
    auto writer = Serializer(dest_memory);

    Asset_Header header;
    header.magic        = Font_Meta.magic;
    header.file_version = Font_Meta.file_version;
    header.asset_type   = Font_Meta.type;
    write(&writer, header);

    auto section = begin_writing_section(&writer, Font_Section.Metrics);
    write(&writer, builder.metrics);
    end_writing_section(&writer, section);

    section = begin_writing_section(&writer, Font_Section.Pixels);
    write(&writer, canvas_width);
    write(&writer, canvas_height);
    copy(canvas_data, eat_array!uint(&writer, canvas_width*canvas_height));
    end_writing_section(&writer, section);

    section = begin_writing_section(&writer, Font_Section.Glyphs);
    uint glyphs_count = atlas.items_count;
    write(&writer, glyphs_count);
    node = atlas.items;
    auto node_i = 0;
    while(node){
        auto entry = cast(Rasterized_Glyph*)node->source;
        write(&writer, entry.glyph);
        node = node->next;
        node_i++;
    }
    assert(node_i == glyphs_count);
    end_writing_section(&writer, section);

    if(kerning_count > 0){
        section = begin_writing_section(&writer, Font_Section.Kerning);
        write(&writer, kerning_count);

        foreach(ref entry; kerning_pairs){
            write(&writer, entry);
        }

        foreach(ref entry; kerning_advance){
            write(&writer, entry);
        }

        end_writing_section(&writer, section);
    }
#endif

    if(font_entry->stroke) FT_Stroker_Done(builder->stroker);
    FT_Done_Face(builder->face);
}

int main(){
    ceabed_begin();

    Font_Builder builder;
    builder.memory = (Buffer){&g_memory[0], Array_Len(g_memory)};
    if(font_builder_begin(&builder)){
        for(u32 entry_index = 0; entry_index < Array_Len(Font_Entries); entry_index++){
            builder.memory.used = 0;
            Font_Entry *entry = &Font_Entries[entry_index];

            const char* src_path = get_path_for_ttf_file(&builder.memory, entry->source_file_name);
            if(src_path){
                if(begin_building_font(&builder, src_path, entry)){
                    for(char c = '!'; c < '~'+1; c++){
                        /*add_codepoint(&builder, c);*/
                    }
                    add_codepoint(&builder, ' ');
                    end_building_font(&builder, entry);
                }
            }
            else{
                fmt_msg("Unable to find path for font {0}", fmt_cstr(entry->source_file_name));
            }
        }
        font_builder_end(&builder);
    }

    ceabed_end();

    return 0;
}
