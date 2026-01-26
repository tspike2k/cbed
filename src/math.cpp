//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "math.hpp"

Vec2 operator+(Vec2 l, Vec2 r){
    Vec2 result = {
        l.x + r.x,
        l.y + r.y,
    };
    return result;
}
