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
constexpr rot2 rot2_identity = rot2(1,0);
