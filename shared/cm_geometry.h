// cm_geometry.h
//

#pragma once

#include "cm_vector.h"

#include <cfloat>

////////////////////////////////////////////////////////////////////////////////
// geometric helper functions

//------------------------------------------------------------------------------
//! Find the intercept time for the given relative position, relative velocity,
//! and maximum change in velocity of the interceptor. Returns the smallest non-
//! negative time to intercept or -FLT_MAX if no interception is possible.
template<typename vec> float intercept_time(
    vec relative_position,
    vec relative_velocity,
    float maximum_velocity)
{
    float a = relative_velocity.dot(relative_velocity) - maximum_velocity * maximum_velocity;
    float b = 2.f * relative_velocity.dot(relative_position);
    float c = (relative_position).dot(relative_position);
    float d = b * b - 4.f * a * c;

    if (d < 0.f) {
        return -FLT_MAX;
    } else if (d == 0.f) {
        float t = -.5f * b / a;
        return t >= 0.f ? t : -FLT_MAX;
    } else {
        float q = -.5f * (b + std::copysign(std::sqrt(d), b));
        auto t = std::minmax({q / a, c / q});
        return t.first >= 0.f ? t.first :
               t.second >= 0.f ? t.second : -FLT_MAX;
    }
}

//------------------------------------------------------------------------------
//! Find the point on line <a,b> neareast to the point c
template<typename vec> vec nearest_point_on_line(vec a, vec b, vec c)
{
    vec v = b - a;
    float num = dot(v, c - a);
    float den = dot(v, v);
    if (num >= den) {
        return b;
    } else if (num <= 0.f) {
        return a;
    } else {
        float t = num / den;
        float s = 1.f - t;
        return a * s + b * t;
    }
}

//------------------------------------------------------------------------------
//! Find the intersection point of line segments <a,b> <c,d> in 2D
inline bool intersect_lines(vec2 a, vec2 b, vec2 c, vec2 d, vec2& p)
{
    vec2 n1 = (b - a).cross(1.f);
    vec2 n2 = (d - c).cross(1.f);

    vec3 u = vec3(n1.x, n1.y, -n1.dot(a)); // 2d plane containing <a,b>
    vec3 v = vec3(n2.x, n2.y, -n2.dot(c)); // 2d plane containing <c,d>
    vec3 w = u.cross(v);

    p = vec2(w.x, w.y) / w.z;

    float d1 = (b - a).dot(p - a); // distance of point along <a,b>
    float d2 = (d - c).dot(p - c); // distance of point along <c,d>

    // check whether point lies within both line segments
    if (d1 < 0.f || d1 > n1.length_sqr()) {
        return false;
    } else if (d2 < 0.f || d2 > n2.length_sqr()) {
        return false;
    } else {
        return true;
    }
}
