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

    double   x;
    double   y;

    rot2() = default;
    explicit rot2(double radians)
        : x(std::cos(radians))
        , y(std::sin(radians))
    {}
    constexpr rot2(double X, double Y) : x(X), y(Y) {}

    bool operator==(rot2 const& R) const { return x == R.x && y == R.y; }
    bool operator!=(rot2 const& R) const { return x != R.x || y != R.y; }
    constexpr double operator[](std::size_t idx) const { return (&x)[idx]; }
    double& operator[](std::size_t idx) { return (&x)[idx]; }

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

    double radians() const { return std::atan2(y, x); }
};

//------------------------------------------------------------------------------
class rot3
{
public:
    static constexpr int dimension = 4;

    double x, y, z, w;

    rot3() = default;
    rot3(vec3 axis, double radians) {
        double c = cos(0.5 * radians), s = sin(0.5 * radians);
        x = s * axis.x;
        y = s * axis.y;
        z = s * axis.z;
        w = c;
    }
    explicit rot3(vec3 axis_angle) {
        double a = length(axis_angle);
        if (a) {
            double c = cos(0.5 * a), s = sin(0.5 * a);
            x = (s / a) * axis_angle.x;
            y = (s / a) * axis_angle.y;
            z = (s / a) * axis_angle.z;
            w = c;
        } else {
            x = y = z = 0; w = 1;
        }
    }
    constexpr rot3(double X, double Y, double Z, double W) : x(X), y(Y), z(Z), w(W) {}

    bool operator==(rot3 const& R) const { return x == R.x && y == R.y && z == R.z && w == R.w; }
    bool operator!=(rot3 const& R) const { return x != R.x || y != R.y || z != R.z || w != R.w; }
    constexpr double operator[](std::size_t idx) const { return (&x)[idx]; }
    double& operator[](std::size_t idx) { return (&x)[idx]; }

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
        double xxzz = R.x * R.x - R.z * R.z;
        double wwyy = R.w * R.w - R.y * R.y;
        double yyxx = R.y * R.y - R.x * R.x;
        double wwzz = R.w * R.w - R.z * R.z;

        double xw2 = R.x * R.w * 2.f;
        double xy2 = R.x * R.y * 2.f;
        double xz2 = R.x * R.z * 2.f;
        double yw2 = R.y * R.w * 2.f;
        double yz2 = R.y * R.z * 2.f;
        double zw2 = R.z * R.w * 2.f;

        return vec3((xxzz + wwyy) * V.x +  (xy2 + zw2)  * V.y +  (xz2 - yw2)  * V.z,
                     (xy2 - zw2)  * V.x + (yyxx + wwzz) * V.y +  (yz2 + xw2)  * V.z,
                     (xz2 + yw2)  * V.x +  (yz2 - xw2)  * V.y + (wwyy - xxzz) * V.z);
    }

    constexpr friend vec3 operator/(vec3 const& V, rot3 const& R) {
        return V * R.inverse();
    }

    constexpr rot3 inverse() const { return rot3(-x, -y, -z, w); }

    double radians() const { return 2.0 * atan2(length(vec3(x, y, z)), w); }

    //! Spherical interpolation of `a` to `b`
    constexpr friend rot3 slerp(rot3 a, rot3 b, double t) {
        if (t <= 0.0) {
            return a;
        } else if (t >= 1.0) {
            return b;
        } else {
            double cosom = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            if (cosom < 0.0) {
                a = rot3(-a.x, -a.y, -a.z, -a.w);
                cosom = -cosom;
            }

            if (cosom < (1.0 - 1e-6)) {
                double sinom2 = 1.0 - cosom * cosom;
                double invsinom = 1.0 / std::sqrt(sinom2);
                double omega = std::atan2(sinom2 * invsinom, cosom);
                double s0 = std::sin((1.0 - t) * omega) * invsinom;
                double s1 = std::sin(t * omega) * invsinom;

                return rot3(s0 * a.x + s1 * b.x,
                            s0 * a.y + s1 * b.y,
                            s0 * a.z + s1 * b.z,
                            s0 * a.w + s1 * b.w);
            } else {
                double s = 1.0 - t;
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
