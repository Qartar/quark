// cm_clothoid.h
//

#pragma once

#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
namespace clothoid {

//------------------------------------------------------------------------------
enum segment_type {
    line,
    arc,
    transition,
};

//------------------------------------------------------------------------------
class segment
{
public:
    segment() = default;

    static segment from_line(vec2 position, vec2 direction, float length) {
        return segment(position, direction, length, 0, 0);
    }
    static segment from_arc(vec2 position, vec2 direction, float length, float curvature) {
        return segment(position, direction, length, curvature, curvature);
    }
    static segment from_transition(vec2 position, vec2 direction, float length, float initial_curvature, float final_curvature) {
        return segment(position, direction, length, initial_curvature, final_curvature);
    }

    //
    // Properties
    //

    segment_type type() const;
    vec2 initial_position() const { return _initial_position; }
    vec2 initial_tangent() const { return _initial_tangent; }
    float length() const { return 1.f / _inverse_length; }
    float initial_curvature() const { return _initial_curvature; }
    vec2 final_position() const;
    vec2 final_tangent() const;
    float final_curvature() const { return _final_curvature; }

    //
    // Derivatives
    //

    //! Evaluate position of the spline segment with respect to arclength `s`
    vec2 evaluate(float s) const;

    //! Evaluate tangent of the spline segment with respect to arclength `s`
    vec2 evaluate_tangent(float s) const;

    //! Evaluate curvature of the spline segment with respect to arclength `s`
    float evaluate_curvature(float s) const;

private:
    vec2 _initial_position;
    vec2 _initial_tangent;
    float _inverse_length;
    float _initial_curvature;
    float _final_curvature;
    // can fit another 4-bytes here for alignment

    static constexpr float _pi = 3.14159265358979323846f;
    static constexpr float _half_pi = 1.57079632679489661923f;

private:
    segment(vec2 position, vec2 direction, float length, float initial_curvature, float final_curvature)
        : _initial_position(position)
        , _initial_tangent(direction)
        , _inverse_length(1.f / length)
        , _initial_curvature(initial_curvature)
        , _final_curvature(final_curvature)
    {}

    void fresnel_integral(float x, float&c, float& s) const;
    void fresnel_integral(double x, double* c, double* s) const;
};

//------------------------------------------------------------------------------
inline segment_type segment::type() const
{
    if (!_initial_curvature && !_final_curvature) {
        return segment_type::line;
    } else if (_initial_curvature == _final_curvature) {
        return segment_type::arc;
    } else {
        return segment_type::transition;
    }
}

//------------------------------------------------------------------------------
inline vec2 segment::final_position() const
{
    switch (type()) {
        case segment_type::line:
            return _initial_position + _initial_tangent / _inverse_length;

        case segment_type::arc:
        case segment_type::transition:
            return evaluate(1 / _inverse_length);

        default:
            __assume(false);
    }
}

//------------------------------------------------------------------------------
inline vec2 segment::final_tangent() const
{
    switch (type()) {
        case segment_type::line:
            return _initial_tangent;

        case segment_type::arc:
        case segment_type::transition:
            return evaluate_tangent(1 / _inverse_length);

        default:
            __assume(false);
    }
}

//------------------------------------------------------------------------------
inline vec2 segment::evaluate(float s) const
{
    switch (type()) {
        case segment_type::line: {
            return _initial_position + _initial_tangent * s;
        }

        case segment_type::arc: {
            vec2 p0 = _initial_position;
            vec2 x0 = _initial_tangent / _initial_curvature;
            vec2 y0 = x0.cross(-1);
            float k0 = _initial_curvature;
            // Note that x0,y0 are pre-multiplied by the radius (1/k)
            return p0 + x0 * sin(s * k0)
                      + y0 * (1.f - cos(s * k0));
        }

        case segment_type::transition: {
            float B = sqrt(1.f / (_pi * abs(_final_curvature - _initial_curvature) * _inverse_length));
            float ts = B * evaluate_curvature(s);
            float t1 = B * evaluate_curvature(0);
            vec2 p1 = _initial_position;
            // If the direction of the clothoid is reversed (k_a > k_b) then the
            // tangent also needs to be reversed.
            vec2 x1 = _initial_tangent * copysign(1.f, _final_curvature - _initial_curvature);
            vec2 y1 = _initial_tangent.cross(-1);
            // If initial curvature is non-zero we need to find the basis vectors
            // of the clothoid segment at the initial point and untransform the
            // given initial tangent by that basis to get the initial tangent of
            // the full clothoid spiral (starting from t=0)
            if (t1) {
                // Find the basis vectors at t=0
                float c1, s1;
                c1 = cos(_half_pi * t1 * t1);
                s1 = sin(_half_pi * t1 * t1);
                vec2 x0 = x1 * c1 - y1 * s1;
                vec2 y0 = x1 * s1 + y1 * c1;
                // Find the position of the clothoid at t=0
                fresnel_integral(t1, c1, s1);
                vec2 p0 = p1 - _pi * B * (x0 * c1 + y0 * s1);
                // Find the position of the clothoid at t_s
                float cs, ss;
                fresnel_integral(ts, cs, ss);
                return p0 + _pi * B * (x0 * cs + y0 * ss);
            } else {
                // Find the position of the clothoid at t_s
                float cs, ss;
                fresnel_integral(ts, cs, ss);
                return p1 + _pi * B * (x1 * cs + y1 * ss);
            }
        }

        default:
            __assume(false);
    }
}

//------------------------------------------------------------------------------
inline vec2 segment::evaluate_tangent(float s) const
{
    switch (type()) {
        case segment_type::line: {
            return _initial_tangent;
        }

        case segment_type::arc: {
            vec2 x0 = _initial_tangent;
            vec2 y0 = _initial_tangent.cross(-1);
            float k0 = _initial_curvature;
            return x0 * cos(s * k0) + y0 * sin(s * k0);
        }

        case segment_type::transition: {
            float B = sqrt(1.f / (_pi * abs(_final_curvature - _initial_curvature) * _inverse_length));
            float ts = B * evaluate_curvature(s);
            float t1 = B * evaluate_curvature(0);
            vec2 x1 = _initial_tangent;
            // If the direction of the clothoid is reversed (k_a > k_b) then the
            // normal also needs to be reversed. Note that unlike the corresponding
            // code in `evaluate` where the tangent is used to calculate the
            // position, the tangent here needs to be consistent with the initial
            // tangent of the segment so only the normal is reversed.
            vec2 y1 = _initial_tangent.cross(copysign(1.f, _initial_curvature - _final_curvature));
            // If initial curvature is non-zero we need to find the basis vectors
            // of the clothoid segment at the initial point and untransform the
            // given initial tangent by that basis to get the initial tangent of
            // the full clothoid spiral (starting from t=0)
            if (t1) {
                // Find the basis vectors at t=0
                float c1, s1;
                c1 = cos(_half_pi * t1 * t1);
                s1 = sin(_half_pi * t1 * t1);
                vec2 x0 = x1 * c1 - y1 * s1;
                vec2 y0 = x1 * s1 + y1 * c1;
                // Find the tangent vector at t_s
                return x0 * cos(_half_pi * ts * ts)
                     + y0 * sin(_half_pi * ts * ts);
            } else {
                // Find the tangent vector at t_s
                return x1 * cos(_half_pi * ts * ts)
                     + y1 * sin(_half_pi * ts * ts);
            }
        }

        default:
            __assume(false);
    }
}

//------------------------------------------------------------------------------
inline float segment::evaluate_curvature(float s) const
{
    switch (type()) {
        case segment_type::line: {
            return 0;
        }

        case segment_type::arc: {
            return _initial_curvature;
        }

        case segment_type::transition: {
            return _initial_curvature + (_final_curvature - _initial_curvature) * s * _inverse_length;
        }

        default:
            __assume(false);
    }
}

//------------------------------------------------------------------------------
inline void segment::fresnel_integral(float x, float& c, float& s) const
{
    double C, S;
    fresnel_integral(x, &C, &S);
    c = float(C);
    s = float(S);
}

} // namespace clothoid
