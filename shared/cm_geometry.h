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
    float a = relative_velocity.dot(relative_velocity) - square(maximum_velocity);
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
