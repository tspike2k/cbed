//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_ATLAS
#define CEABED_ATLAS

#include "common.h"

// These are functions and types used to construct texture atlases. These will usually be used
// internally by offline utilities.

typedef struct Atlas_Node Atlas_Node;

struct Atlas_Node{
    Atlas_Node *next;
    u32   x;     // X grows to the right
    u32   y;     // Y grows to the bottom
    u32   width;
    u32   height;
    void *source;
};

typedef struct{
    Buffer*    memory;
    u32        canvas_width;
    u32        canvas_height;

    Atlas_Node* items;
    u32         items_count;
    u32         items_width;  // Total width of all items combined
    u32         items_height; // Total height of all items combined
} Atlas_Packer;

#endif // CEABED_ATLAS
