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
//! negative time to intercept or -DBL_MAX if no interception is possible.
template<typename vec> double intercept_time(
    vec relative_position,
    vec relative_velocity,
    float maximum_velocity)
{
    double a = relative_velocity.dot(relative_velocity) - maximum_velocity * maximum_velocity;
    double b = 2.0 * relative_velocity.dot(relative_position);
    double c = (relative_position).dot(relative_position);
    double d = b * b - 4.0 * a * c;

    if (d < 0.0) {
        return -DBL_MAX;
    } else if (d == 0.0) {
        double t = -0.5 * b / a;
        return t >= 0.0 ? t : -DBL_MAX;
    } else {
        double q = -0.5 * (b + std::copysign(std::sqrt(d), b));
        auto t = std::minmax({q / a, c / q});
        return t.first >= 0.0 ? t.first :
               t.second >= 0.0 ? t.second : -DBL_MAX;
    }
}
