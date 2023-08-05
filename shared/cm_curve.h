// cm_curve.h
//

#pragma once

#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
namespace curve {

//------------------------------------------------------------------------------
class cubic
{
public:
    vec2 a, b, c, d;

public:
    static cubic from_bezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3) {
        return {
            -p0 + 3 * p1 - 3 * p2 + p3,
            3 * p0 - 6 * p1 + 3 * p2,
            -3 * p0 + 3 * p1,
            p0,
        };
    }

    vec2 evaluate(float t) const {
        // P(t) = at^3 + bt^2 + ct + d
        return ((a * t + b) * t + c) * t + d;
    }

    vec2 evaluate_tangent(float t) const {
        // P'(t) = 3at^2 + 2bt + c
        return (3 * a * t + 2 * b) * t + c;
    }

    float evaluate_curvature(float t) const {
        // P"(t) = 6at + 2b
        vec2 uu = 6 * a * t + 2 * b;
        vec2 u = evaluate_tangent(t);
        return cross(u, uu) / (length(u) * length_sqr(u));
    }

    float evaluate_arclength(float t) const {
        vec2 p1 = evaluate(t);
        return evaluate_arclength_r(0.f, t, d, p1, length(p1 - d));
    }

    float evaluate_arclength_r(float t0, float t1, vec2 p0, vec2 p1, float s0) const {
        float tm = .5f * (t0 + t1);
        vec2 pm = evaluate(tm);
        float s1 = length(pm - p0);
        float s2 = length(p1 - pm);
        if ((s1 + s2) <= s0 * 1.001f) {
            return s1 + s2;
        }
        else {
            return evaluate_arclength_r(t0, tm, p0, pm, s1)
                 + evaluate_arclength_r(tm, t1, pm, p1, s2);
        }
    }
};

} // namespace curve
