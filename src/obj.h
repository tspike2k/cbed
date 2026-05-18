//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_OBJ_H
#define CEABED_OBJ_H

#include "common.h"
#include "math.h"

enum{
    Obj_Flag_Texture  = (1 << 0),
    Obj_Flag_Normal   = (1 << 1),
    Obj_Flag_Color    = (1 << 2)
};

// The texture, normal and color fields are all optional; test Obj_Polygon.flags to see if they
// are defined for this polygon.
typedef struct{
    Vec4 position;
    Vec3 texture;
    Vec3 normal;
    Vec3 color;
} Obj_Vertex;

typedef struct{
    u32        flags;
    u32        vertex_count;
    Obj_Vertex vertex[4];
} Obj_Polygon;

struct Obj_Data{
    // TODO: Should faces be seperated by groups/material/etc?
    size_t faces_count;
    u8 internal[128];
};

Ceabed_API bool obj_parse_file(Obj_Data *obj, const char *file_name, Buffer *dest, Buffer* scratch);
Ceabed_API Obj_Polygon obj_polygon_from_face(Obj_Data *obj, size_t face_index);

#endif // CEABED_OBJ_H
