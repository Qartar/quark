// cm_curve.h
//

#pragma once

#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
namespace curve {

//------------------------------------------------------------------------------
//! Gauss-Legendre quadrature integration from a to b with n=7
template<typename Fn> float gauss_legendre_7(Fn fn, float a, float b)
{
    constexpr float w[] = {
        +0.4179591836734694f,
        +0.3818300505051189f,
        +0.3818300505051189f,
        +0.2797053914892766f,
        +0.2797053914892766f,
        +0.1294849661688697f,
        +0.1294849661688697f,
    };
    constexpr float x[] = {
        +0.f,
        -0.4058451513773972f,
        +0.4058451513773972f,
        -0.7415311855993945f,
        +0.7415311855993945f,
        -0.9491079123427585f,
        +0.9491079123427585f,
    };

    float c = .5f * (b - a);
    float d = .5f * (b + a);

    double s = 0.f;
    for (int ii = 0; ii < 7; ++ii) {
        s += w[ii] * fn(x[ii] * c + d);
    }
    return c * (float)s;
}

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

    void split(cubic &q, cubic &r, float t) const {
        float s = 1.f - t;

        vec2 p0 = d;
        vec2 p1 = d + c * (1.f / 3.f);
        vec2 p2 = d + c * (2.f / 3.f) + b * (1.f / 3.f);
        vec2 p3 = d + c + b + a;

        vec2 pm = p1 * s + p2 * t;

        vec2 q0 = p0;
        vec2 q1 = p0 * s + p1 * t;
        vec2 q2 = q1 * s + pm * t;

        vec2 r3 = p3;
        vec2 r2 = p2 * s + p3 * t;
        vec2 r1 = pm * s + r2 * t;

        vec2 r0 = q2 * s + r1 * t;
        vec2 q3 = r0;

        q = from_bezier(q0, q1, q2, q3);
        r = from_bezier(r0, r1, r2, r3);
    }

    void split(cubic& p, cubic& q, cubic& r, float t1, float t2) const {
        cubic p2;
        split(p, p2, t1);
        p2.split(q, r, (t2 - t1) / (1.f - t1));
    }

    int calculate_inflection_points(float& t1, float& t2) const {
        // https://faculty.runi.ac.il/arik/quality/appendixa.html
        const float eps0 = 1e-4f;
        const float eps1 = 1.f - eps0;
        float qa = (1.f / 3.f) * cross(b, a);
        float qb = (1.f / 3.f) * cross(c, a);
        float qc = (1.f / 9.f) * cross(c, b);
        // solve quadratic
        float det = qb * qb - 4.f * qa * qc;
        if (det < 0.f) {
            return 0;
        } else if (det == 0.f) {
            t1 = -.5f * qb / qa;
            if (t1 < eps0 || t1 > eps1) {
                return 0;
            } else {
                return 1;
            }
        } else {
            float q = -.5f * (qb + std::copysign(std::sqrt(det), qb));
            auto t = std::minmax({q / qa, qc / q});
            if (t.first > eps1 || t.second < eps0 || (t.first < eps0 && t.second > eps1)) {
                return 0;
            } else if (t.first < eps0) {
                t1 = t.second;
                return 1;
            } else if (t.second > eps1) {
                t1 = t.first;
                return 1;
            } else {
                t1 = t.first;
                t2 = t.second;
                return 2;
            }
        }
    }

    float evaluate_arclength(float t) const {
        cubic q, r, s;
        float t1, t2;
        int n = calculate_inflection_points(t1, t2);
        if (n == 2) {
            split(q, r, s, t1, t2);
            if (t1 > t) {
                n = 0;
                t /= t1;
            } else if (t2 > t) {
                n = 1;
                t = (t - t1) / (t2 - t1);
            } else {
                t = (t - t2) / (1.f - t2);
            }
        } else if (n == 1) {
            split(q, r, t1);
            if (t1 > t) {
                n = 0;
                t /= t1;
            } else {
                t = (t - t1) / (1.f - t1);
            }
        } else {
            q = *this;
        }

        if (n == 2) {
            return gauss_legendre_7([&q](float u){return q.evaluate_tangent(u).length();}, 0.f, 1.f)
                 + gauss_legendre_7([&r](float u){return r.evaluate_tangent(u).length();}, 0.f, 1.f)
                 + gauss_legendre_7([&s](float u){return s.evaluate_tangent(u).length();}, 0.f, t);
        } else if (n == 1) {
            return gauss_legendre_7([&q](float u){return q.evaluate_tangent(u).length();}, 0.f, 1.f)
                 + gauss_legendre_7([&r](float u){return r.evaluate_tangent(u).length();}, 0.f, t);
        } else {
            return gauss_legendre_7([&q](float u){return q.evaluate_tangent(u).length();}, 0.f, t);
        }
    }
};

} // namespace curve
