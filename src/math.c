//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2024
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "math.h"

void clampf(f32 *value, f32 min_val, f32 max_val){
    *value = *value < min_val ? min_val : (*value > max_val ? max_val : *value);
}

#define Vec_OP(c, op) (a.c op b.c)

Ceabed_API Vec2 v2(f32 x, f32 y){
    Vec2 result = {
        x, y
    };
    return result;
}

Ceabed_API Vec2 v2_sub(Vec2 a, Vec2 b){
    Vec2 result = {
        Vec_OP(x, -),
        Vec_OP(y, -),
    };
    return result;
}

Ceabed_API Vec2 v2_add(Vec2 a, Vec2 b){
    Vec2 result = {
        Vec_OP(x, +),
        Vec_OP(y, +),
    };
    return result;
}

Ceabed_API Vec2 v2_mul(Vec2 a, Vec2 b){
    Vec2 result = {
        Vec_OP(x, *),
        Vec_OP(y, *),
    };
    return result;
}

Ceabed_API Vec2 v2_muls(Vec2 a, float b){
    Vec2 result = {
        a.x * b,
        a.y * b,
    };
    return result;
}

Ceabed_API Vec2 v2_div(Vec2 a, Vec2 b){
    Vec2 result = {
        Vec_OP(x, /),
        Vec_OP(y, /),
    };
    return result;
}

Ceabed_API Vec3 v3_sub(Vec3 a, Vec3 b){
    Vec3 result = {
        Vec_OP(x, -),
        Vec_OP(y, -),
        Vec_OP(z, -),
    };
    return result;
}

Ceabed_API Vec3 v3_add(Vec3 a, Vec3 b){
    Vec3 result = {
        Vec_OP(x, +),
        Vec_OP(y, +),
        Vec_OP(z, +),
    };
    return result;
}

Ceabed_API Vec3 v3_mul(Vec3 a, Vec3 b){
    Vec3 result = {
        Vec_OP(x, *),
        Vec_OP(y, *),
        Vec_OP(z, *),
    };
    return result;
}

Ceabed_API Vec3 v3_muls(Vec3 a, float b){
    Vec3 result = {
        a.x * b,
        a.y * b,
        a.z * b,
    };
    return result;
}

Ceabed_API Vec3 v3_div(Vec3 a, Vec3 b){
    Vec3 result = {
        Vec_OP(x, /),
        Vec_OP(y, /),
        Vec_OP(z, /),
    };
    return result;
}

#undef Vec_OP

Ceabed_API Vec2 v2_normalize(Vec2 v){
    float magnitude = sqrt(v.x * v.x + v.y * v.y);

    Vec2 result = {0.0f, 0.0f};
    if(magnitude != 0.0f){
        result.x = v.x / magnitude;
        result.y = v.y / magnitude;
    }

    return result;
}

Ceabed_API Vec3 v3_normalize(Vec3 a){
    float mag = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);

    Vec3 result = {0.0f, 0.0f, 0.0f};
    if(mag > 0.00001f){ // TODO: Better epsilon?
        result = (Vec3){
            a.x / mag,
            a.y / mag,
            a.z / mag
        };
    }
    return result;
}

Ceabed_API float v2_squared(Vec2 v){
    float result = v.x*v.x + v.y*v.y;
    return result;
}

Ceabed_API float v3_squared(Vec3 v){
    float result = v.x*v.x + v.y*v.y + v.z*v.z;
    return result;
}

Ceabed_API float v2_length(Vec2 v){
    float result = sqrt(v.x * v.x + v.y * v.y);
    return result;
}

Ceabed_API float v3_length(Vec3 v){
    float result = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return result;
}

Ceabed_API float v2_dot(Vec2 a, Vec2 b){
    float result = a.x*b.x + a.y*b.y;
    return result;
}

Ceabed_API float v3_dot(Vec3 a, Vec3 b){
    float result = a.x*b.x + a.y*b.y + a.z*b.z;
    return result;
}

Ceabed_API Vec3 cross(Vec3 a, Vec3 b){
    Vec3 result = {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
    return result;
}

Ceabed_API float v2_distance_between(Vec2 a, Vec2 b){
    Vec2 diff = v2_sub(a, b);
    float result = sqrt(diff.x * diff.x + diff.y * diff.y);
    return result;
}

Ceabed_API float squared(float n){
    float result = n*n;
    return result;
}

Ceabed_API float v2_dist_sq(Vec2 a, Vec2 b){
    Vec2 c = v2_sub(b, a);
    float result = v2_dot(c, c);
    return result;
}

Ceabed_API float v3_dist_sq(Vec3 a, Vec3 b){
    Vec3 c = v3_sub(b, a);
    float result = v3_dot(c, c);
    return result;
}

Ceabed_API float lerp(float start, float end, float t){
    float result = (end * t) + (start * (1.0f - t));
    return result;
}

Ceabed_API Vec4 v4(f32 x, f32 y, f32 z, f32 w){
    Vec4 result = {{x, y, z, w}};
    return result;
}

Ceabed_API Vec4 v4_lerp(Vec4 a, Vec4 b, float t){
    Vec4 result = {{
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
        lerp(a.w, b.w, t),
    }};
    return result;
}

Ceabed_API Rect rect_from_min_wh(Vec2 min_p, float w, float h){
    Vec2 extents = {w*0.5f, h*0.5f};
    Vec2 center  = {min_p.x + extents.x, min_p.y + extents.y};
    Rect result  = {center, extents};
    return result;
}

Ceabed_API Rect rect_from_min_max(Vec2 min_p, Vec2 max_p){
    Vec2 extents = {(max_p.x - min_p.x)*0.5f, (max_p.y - min_p.y)*0.5f};
    Vec2 center  = {min_p.x + extents.x, min_p.y + extents.y};
    Rect result = {center, extents};
    return result;
}

Ceabed_API Vec2 rect_min(Rect r){
    Vec2 result = {r.center.x - r.extents.x, r.center.y - r.extents.y};
    return result;
}

Ceabed_API Vec2 rect_max(Rect r){
    Vec2 result = {r.center.x + r.extents.x, r.center.y + r.extents.y};
    return result;
}

Ceabed_API float rect_width(Rect r){
    float result = r.extents.x*2.0f;
    return result;
}

Ceabed_API float rect_height(Rect r){
    float result = r.extents.y*2.0f;
    return result;
}

Ceabed_API bool is_point_inside_rect(Vec2 p, Rect r){
    Vec2 r_min = rect_min(r);
    Vec2 r_max = rect_max(r);
    bool result = p.x > r_min.x && p.x < r_max.x
               && p.y > r_min.y && p.y < r_max.y;
    return result;
}

Ceabed_API Mat4 mat4_mul(Mat4 a, Mat4 b){
    Mat4 result;

#define Mat4_OP(r, c) result.m[r][c] = a.m[r][0]*b.m[0][c] + a.m[r][1]*b.m[1][c] + a.m[r][2]*b.m[2][c] + a.m[r][3]*b.m[3][c]
    Mat4_OP(0, 0);
    Mat4_OP(0, 1);
    Mat4_OP(0, 2);
    Mat4_OP(0, 3);
    Mat4_OP(1, 0);
    Mat4_OP(1, 1);
    Mat4_OP(1, 2);
    Mat4_OP(1, 3);
    Mat4_OP(2, 0);
    Mat4_OP(2, 1);
    Mat4_OP(2, 2);
    Mat4_OP(2, 3);
    Mat4_OP(3, 0);
    Mat4_OP(3, 1);
    Mat4_OP(3, 2);
    Mat4_OP(3, 3);
#undef Mat4_OP

    return result;
}

Ceabed_API Vec4 mat4_mul_v4(Mat4 a, Vec4 v){
#define Mat4_OP(r) a.m[r][0]*v.x + a.m[r][1]*v.y + a.m[r][2]*v.z + a.m[r][3]*v.w
    Vec4 result;
    result.x = Mat4_OP(0);
    result.y = Mat4_OP(1);
    result.z = Mat4_OP(2);
    result.w = Mat4_OP(3);

    return result;
#undef Mat4_OP
}

Ceabed_API Mat4 mat4_transpose(Mat4 a){
    Mat4 result;
#define Mat4_OP(r, c) result.m[r][c] = a.m[c][r];
    Mat4_OP(0, 0);
    Mat4_OP(0, 1);
    Mat4_OP(0, 2);
    Mat4_OP(0, 3);
    Mat4_OP(1, 0);
    Mat4_OP(1, 1);
    Mat4_OP(1, 2);
    Mat4_OP(1, 3);
    Mat4_OP(2, 0);
    Mat4_OP(2, 1);
    Mat4_OP(2, 2);
    Mat4_OP(2, 3);
    Mat4_OP(3, 0);
    Mat4_OP(3, 1);
    Mat4_OP(3, 2);
    Mat4_OP(3, 3);
#undef Mat4_OP

    return result;
}

Ceabed_API Mat4 mat4_scale(Vec3 s){
    Mat4 result = {{
        {s.x,   0,    0,   0},
        {  0, s.y,    0,   0},
        {  0,   0,  s.z,   0},
        {  0,   0,    0,   1},
    }};
    return result;
}

Ceabed_API Mat4 mat4_translate(Vec3 offset){
    Mat4 result = {{
        {1.0f, 0.0f, 0.0f, offset.x},
        {0.0f, 1.0f, 0.0f, offset.y},
        {0.0f, 0.0f, 1.0f, offset.z},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }};
    return result;
}

Ceabed_API Mat4 mat4_rot_x(float angle_rad){
    float c = cos(angle_rad);
    float s = sin(angle_rad);

    Mat4 result = {{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, c,    -s,   0.0f},
        {0.0f, s,     c,   0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return result;
}

Ceabed_API Mat4 mat4_rot_y(float angle_rad){
    float c = cos(angle_rad);
    float s = sin(angle_rad);

    Mat4 result = {{
        {c,    0.0f, s,    0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {-s,   0.0f, c,    0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return result;
}

Ceabed_API Mat4 mat4_rot_z(float angle_rad){
    float c = cos(angle_rad);
    float s = sin(angle_rad);

    Mat4 result = {{
        {c,   -s,    0.0f, 0.0f},
        {s,    c,    0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return result;
}

Ceabed_API u64 round_up_power_of_two(u64 n){
    // NOTE: Adapted from here:
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    n--;
    for(u32 byte_index = 0; byte_index < sizeof(u64); byte_index++){
        n |= n >> ((u64)pow(2, byte_index)); // ^^ is the pow operator in D
    }
    n++;
    return n;
}

Ceabed_API float deg_to_rad(float degrees){
    float result = degrees*(PI/180.0f);
    return result;
}


Ceabed_API Vec3 polar_to_world(Vec3 polar, Vec3 target_pos){
    float phi   = polar.x * (PI/180.0f);
    float theta = (polar.y + 90.0f) * (PI/180.0f);

    float phi_sin = sin(phi);
    float phi_cos = cos(phi);
    float theta_sin = sin(theta);
    float theta_cos = cos(theta);

    Vec3 dir_to_camera = (Vec3){theta_sin * phi_cos, theta_cos, theta_sin*phi_sin};
    Vec3 world_pos = v3_add(target_pos, v3_muls(dir_to_camera, polar.z));
    return world_pos;
}
