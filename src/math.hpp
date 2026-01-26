//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#ifndef CEABED_MATH_HPP
#define CEABED_MATH_HPP

#include "math.h"

template<typename T> T min(T a, T b){
    T result = MIN(a, b);
    return result;
}

template<typename T> T max(T a, T b){
    T result = MAX(a, b);
    return result;
}

template<typename T> void clamp(T* a, T min_val, T max_val){
    *a = CLAMP(*a, min_val, max_val);
}

Vec2 operator+(Vec2 l, Vec2 r);

#endif // CEABED_MATH_HPP
