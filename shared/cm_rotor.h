// cm_rotor.h
//

#pragma once

#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
// rotor types

//------------------------------------------------------------------------------
class rot2
{
public:
    static constexpr int dimension = 2;

    float   x;
    float   y;

    rot2() = default;
    explicit rot2(float radians)
        : x(std::cos(radians))
        , y(std::sin(radians))
    {}
    constexpr rot2(float X, float Y) : x(X), y(Y) {}

    bool operator==(rot2 const& R) const { return x == R.x && y == R.y; }
    bool operator!=(rot2 const& R) const { return x != R.x || y != R.y; }
    constexpr float operator[](std::size_t idx) const { return (&x)[idx]; }
    float& operator[](std::size_t idx) { return (&x)[idx]; }

    constexpr rot2 operator*(rot2 const& R) const {
        return rot2(x * R.x - y * R.y, x * R.y + y * R.x);
    }

    constexpr rot2 operator/(rot2 const& R) const {
        return *this * R.inverse();
    }

    constexpr friend vec2 operator*(vec2 const& V, rot2 const& R) {
        return vec2(V.x * R.x - V.y * R.y, V.x * R.y + V.y * R.x);
    }

    constexpr friend vec2 operator/(vec2 const& V, rot2 const& R) {
        return V * R.inverse();
    }

    constexpr rot2 inverse() const { return rot2(x, -y); }

    float radians() const { return std::atan2(y, x); }
};

//------------------------------------------------------------------------------
class rot3
{
public:
    static constexpr int dimension = 4;

    float   x;
    float   y;
    float   z;
    float   w;

    rot3() = default;
    rot3(vec3 axis, float radians) {
        float c = cos(.5f * radians), s = sin(.5f * radians);
        x = s * axis.x;
        y = s * axis.y;
        z = s * axis.z;
        w = c;
    }
    explicit rot3(vec3 axis_angle) {
        float a = length(axis_angle);
        if (a) {
            float c = cos(.5f * a), s = sin(.5f * a);
            x = (s / a) * axis_angle.x;
            y = (s / a) * axis_angle.y;
            z = (s / a) * axis_angle.z;
            w = c;
        } else {
            x = y = z = 0; w = 1;
        }
    }
    constexpr rot3(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {}

    bool operator==(rot3 const& R) const { return x == R.x && y == R.y && z == R.z && w == R.w; }
    bool operator!=(rot3 const& R) const { return x != R.x || y != R.y || z != R.z || w != R.w; }
    constexpr float operator[](std::size_t idx) const { return (&x)[idx]; }
    float& operator[](std::size_t idx) { return (&x)[idx]; }

    constexpr rot3 operator*(rot3 const& R) const {
        return rot3(w * R.x + x * R.w + y * R.z - z * R.y,
                    w * R.y + y * R.w + z * R.x - x * R.z,
                    w * R.z + z * R.w + x * R.y - y * R.x,
                    w * R.w - x * R.x - y * R.y - z * R.z);
    }

    constexpr rot3 operator/(rot3 const& R) const {
        return *this * R.inverse();
    }

    constexpr friend vec3 operator*(vec3 const& V, rot3 const& R) {
        float xxzz = R.x * R.x - R.z * R.z;
        float wwyy = R.w * R.w - R.y * R.y;
        float yyxx = R.y * R.y - R.x * R.x;
        float wwzz = R.w * R.w - R.z * R.z;

        float xw2 = R.x * R.w * 2.f;
        float xy2 = R.x * R.y * 2.f;
        float xz2 = R.x * R.z * 2.f;
        float yw2 = R.y * R.w * 2.f;
        float yz2 = R.y * R.z * 2.f;
        float zw2 = R.z * R.w * 2.f;

        return vec3((xxzz + wwyy) * V.x +  (xy2 + zw2)  * V.y +  (xz2 - yw2)  * V.z,
                     (xy2 - zw2)  * V.x + (yyxx + wwzz) * V.y +  (yz2 + xw2)  * V.z,
                     (xz2 + yw2)  * V.x +  (yz2 - xw2)  * V.y + (wwyy - xxzz) * V.z);
    }

    constexpr friend vec3 operator/(vec3 const& V, rot3 const& R) {
        return V * R.inverse();
    }

    constexpr rot3 inverse() const { return rot3(-x, -y, -z, w); }

    float radians() const { return 2.f * atan2f(length(vec3(x, y, z)), w); }

    //! Spherical interpolation of `a` to `b`
    constexpr friend rot3 slerp(rot3 a, rot3 b, float t) {
        if (t <= 0.f) {
            return a;
        } else if (t >= 1.f) {
            return b;
        } else {
            float cosom = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            if (cosom < 0.f) {
                a = rot3(-a.x, -a.y, -a.z, -a.w);
                cosom = -cosom;
            }

            if (cosom < (1.f - 1e-6f)) {
                float sinom2 = 1.f - cosom * cosom;
                float invsinom = 1.f / std::sqrt(sinom2);
                float omega = std::atan2(sinom2 * invsinom, cosom);
                float s0 = std::sin((1.f - t) * omega) * invsinom;
                float s1 = std::sin(t * omega) * invsinom;

                return rot3(s0 * a.x + s1 * b.x,
                            s0 * a.y + s1 * b.y,
                            s0 * a.z + s1 * b.z,
                            s0 * a.w + s1 * b.w);
            } else {
                float s = 1.f - t;
                return rot3(s * a.x + t * b.x,
                            s * a.y + t * b.y,
                            s * a.z + t * b.z,
                            s * a.w + t * b.w);
            }
        }
    }
};

//------------------------------------------------------------------------------
constexpr rot2 rot2_identity = rot2(1,0);
constexpr rot3 rot3_identity = rot3(0,0,0,0);
