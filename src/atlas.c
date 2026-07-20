//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "atlas.h"

Cbed_API Atlas_Packer atlas_packer_begin(Buffer *memory){
    Atlas_Packer packer = {0};
    packer.memory = memory;
    return packer;
}

Cbed_API void atlas_packer_add(Atlas_Packer *packer, u32 width, u32 height, void *source){
    Atlas_Node *node = buffer_push_type(Atlas_Node, packer->memory);

    node->next = packer->items;
    packer->items = node;

    node->width  = width;
    node->height = height;
    node->source = source;

    packer->items_count++;
    packer->items_width  += width;
    packer->items_height += height;
}

Cbed_API void atlas_packer_end(Atlas_Packer* packer, u32 padding, bool use_powers_of_two){
    if(packer->items_count == 0) return;

    // Using this algorithm, we estimate the desired canvas width and grow the height as
    // much as we need.
    //
    // TODO: Perhaps there's a better way to handle this? It seems to work pretty well so far.
    // We may need to use maximum item width rather than everage item width for things other
    // than fonts.
    u32 columns = (u32)ceil(sqrt(packer->items_count));
    u32 average_width  = packer->items_width  / packer->items_count;
    u32 canvas_width   = (average_width  * columns) + 2*padding*(packer->items_count+1);

    if(use_powers_of_two){
        canvas_width  = round_up_power_of_two(canvas_width);
    }

    u32 pen_x = padding;
    u32 pen_y = padding;

    auto node = packer->items;

    u32 canvas_height = 0;
    u32 max_line_height = 0;
    while(node){
        if(pen_x + node->width + padding >= canvas_width){
            pen_y += max_line_height + padding;
            pen_x = padding;
            max_line_height = 0;
        }

        assert(pen_x + node->width + padding < canvas_width);
        node->x = pen_x;
        node->y = pen_y;
        max_line_height = MAX(max_line_height, node->height);
        canvas_height   = MAX(canvas_height, pen_y + max_line_height + padding);

        pen_x += node->width + padding;

        node = node->next;
    }

    if(use_powers_of_two)
        canvas_height = round_up_power_of_two(canvas_height);

    packer->canvas_width  = canvas_width;
    packer->canvas_height = canvas_height;
}
