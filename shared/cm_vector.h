// cm_vector.h
//

#pragma once

#include <cmath>
#include <algorithm>

class vec2;
class vec3;
class vec4;

////////////////////////////////////////////////////////////////////////////////
// vector types

//------------------------------------------------------------------------------
class vec2
{
public:
    float   x;
    float   y;

// constructors

    vec2() = default;
    constexpr vec2(float X, float Y) : x(X), y(Y) {}
    explicit constexpr vec2(float S) : x(S), y(S) {}

    bool operator==(vec2 const& V) const { return x == V.x && y == V.y; }
    bool operator!=(vec2 const& V) const { return x != V.x || y != V.y; }
    constexpr float operator[](std::size_t idx) const { return (&x)[idx]; }
    float& operator[](std::size_t idx) { return (&x)[idx]; }
    operator float*() { return &x; }
    operator float const*() const { return &x; }

// algebraic vector operations

    constexpr vec2 operator-() const {return vec2(-x, -y);}
    constexpr vec2 operator+(vec2 const& V) const { return vec2(x+V.x, y+V.y); }
    constexpr vec2 operator-(vec2 const& V) const { return vec2(x-V.x, y-V.y); }
    constexpr vec2 operator*(vec2 const& V) const { return vec2(x*V.x, y*V.y); }
    constexpr vec2 operator/(vec2 const& V) const { return vec2(x/V.x, y/V.y); }
    constexpr vec2 operator*(float S) const { return vec2(x*S, y*S); }
    constexpr vec2 operator/(float S) const { return vec2(x/S, y/S); }

// algebraic vector assignment operations

    vec2& operator+=(vec2 const& V) { x+=V.x; y+=V.y; return *this; }
    vec2& operator-=(vec2 const& V) { x-=V.x; y-=V.y; return *this; }
    vec2& operator*=(vec2 const& V) { x*=V.x; y*=V.y; return *this; }
    vec2& operator/=(vec2 const& V) { x/=V.x; y/=V.y; return *this; }
    vec2& operator*=(float S) { x*=S; y*=S; return *this; }
    vec2& operator/=(float S) { x/=S; y/=S; return *this; }

// utility functions

    float length() const { return std::sqrt(length_sqr()); }
    float length_sqr() const { return x*x + y*y; }
    vec2 normalize() const { float len = length(); return len ? *this / len : *this; }
    void normalize_self() { float len = length(); if (len) { *this /= len; } }
    float normalize_length() { float len = length(); if (len) { *this /= len; } return len; }
    void clear() { x=0.0f; y=0.0f; }

    constexpr float dot(vec2 const& V) const { return x*V.x + y*V.y; }
    constexpr vec2 cross(float V) const { return vec2(y*V, -x*V); }
    constexpr float cross(vec2 const& V) const { return x*V.y - y*V.x; }
};

//------------------------------------------------------------------------------
class vec3
{
public:
    float   x;
    float   y;
    float   z;

// constructors

    vec3() = default;
    constexpr vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    constexpr explicit vec3(float S) : x(S), y(S), z(S) {}
    constexpr explicit vec3(vec2 const& V, float Z = 0) :x(V.x), y(V.y), z(Z) {}

    bool operator==(vec3 const& V) const {return x == V.x && y == V.y && z == V.z; }
    bool operator!=(vec3 const& V) const {return x != V.x || y != V.y || z != V.z; }
    constexpr float operator[](std::size_t idx) const { return (&x)[idx]; }
    float& operator[](std::size_t idx) { return (&x)[idx]; }
    operator float*() { return &x; }
    operator float const*() const { return &x; }

// algebraic vector operations

    constexpr vec3 operator-() const { return vec3(-x, -y, -z); }
    constexpr vec3 operator+(vec3 const& V) const { return vec3(x+V.x, y+V.y, z+V.z); }
    constexpr vec3 operator-(vec3 const& V) const { return vec3(x-V.x, y-V.y, z-V.z); }
    constexpr vec3 operator*(vec3 const& V) const { return vec3(x*V.x, y*V.y, z*V.z); }
    constexpr vec3 operator/(vec3 const& V) const { return vec3(x/V.x, y/V.y, z/V.z); }
    constexpr vec3 operator*(float S) const { return vec3(x*S, y*S, z*S); }
    constexpr vec3 operator/(float S) const { return vec3(x/S, y/S, z/S); }

// algebraic vector assignment operations

    vec3& operator+=(vec3 const& V) { x+=V.x; y+=V.y; z+=V.z; return *this; }
    vec3& operator-=(vec3 const& V) { x-=V.x; y-=V.y; z-=V.z; return *this; }
    vec3& operator*=(vec3 const& V) { x*=V.x; y*=V.y; z*=V.z; return *this; }
    vec3& operator/=(vec3 const& V) { x/=V.x; y/=V.y; z/=V.z; return *this; }
    vec3& operator*=(float S) { x*=S; y*=S; z*=S; return *this; }
    vec3& operator/=(float S) { x/=S; y/=S; z/=S; return *this; }

// utility functions

    float length() const {return std::sqrt(length_sqr()); }
    float length_sqr() const {return x*x + y*y + z*z; }
    vec3 normalize() const { float len = length(); return len ? *this / len : *this; }
    void normalize_self() { float len = length(); if (len) { *this /= len; } }
    float normalize_length() { float len = length(); if (len) { *this /= len; } return len; }
    void clear() { x=0.0f; y=0.0f; z=0.0f; }

    constexpr float dot(vec3 const& V) const { return x*V.x + y*V.y + z*V.z;}
    constexpr vec3 cross(vec3 const& V) const { return vec3( y*V.z - z*V.y, z*V.x - x*V.z, x*V.y - y*V.x ); }

    constexpr vec2 to_vec2() const { return vec2(x, y); }
};

//------------------------------------------------------------------------------
class vec4
{
public:
    float   x;
    float   y;
    float   z;
    float   w;

// constructors

    vec4() = default;
    constexpr vec4(float X, float Y, float Z, float W = 1) : x(X), y(Y), z(Z), w(W) {}
    constexpr explicit vec4(float S) : x(S), y(S), z(S), w(S) {}
    constexpr explicit vec4(vec3 const& V, float W = 1) : x(V.x), y(V.y), z(V.z), w(W) {}

    bool operator==(vec4 const& V) const { return x==V.x && y==V.y && z==V.z && w==V.w; }
    bool operator!=(vec4 const& V) const { return x!=V.x || y!=V.y || z!=V.z || w!=V.w; }
    constexpr float operator[](std::size_t idx) const { return (&x)[idx]; }
    float& operator[](std::size_t idx) { return (&x)[idx]; }
    operator float*() { return &x; }
    operator float const*() const { return &x; }

// algebraic vector operations

    constexpr vec4 operator-() const { return vec4(-x, -y, -z, -w); }
    constexpr vec4 operator+(vec4 const& V) const { return vec4(x+V.x, y+V.y, z+V.z, w+V.w); }
    constexpr vec4 operator-(vec4 const& V) const { return vec4(x-V.x, y-V.y, z-V.z, w-V.w); }
    constexpr vec4 operator*(vec4 const& V) const { return vec4(x*V.x, y*V.y, z*V.z, w*V.w); }
    constexpr vec4 operator/(vec4 const& V) const { return vec4(x/V.x, y/V.y, z/V.z, w/V.w); }
    constexpr vec4 operator*(float S) const { return vec4(x*S, y*S, z*S, w*S); }
    constexpr vec4 operator/(float S) const { return vec4(x/S, y/S, z/S, w/S); }

// algebraic vector assignment operations

    vec4& operator+=(vec4 const& V) { x+=V.x; y+=V.y; z+=V.z; w+=V.w; return *this; }
    vec4& operator-=(vec4 const& V) { x-=V.x; y-=V.y; z-=V.z; w-=V.w; return *this; }
    vec4& operator*=(vec4 const& V) { x*=V.x; y*=V.y; z*=V.z; w*=V.w; return *this; }
    vec4& operator/=(vec4 const& V) { x/=V.x; y/=V.y; z/=V.z; w/=V.w; return *this; }
    vec4& operator*=(float S) { x*=S; y*=S; z*=S; w*=S; return *this; }
    vec4& operator/=(float S) { x/=S; y/=S; z/=S; w/=S; return *this; }

// utility functions

    float length() const { return std::sqrt(length_sqr()); }
    float length_sqr() const { return x*x + y*y + z*z + w*w; }
    vec4 normalize() const { float len = length(); return len ? *this / len : *this; }
    void normalize_self() { float len = length(); if (len) { *this /= len; } }
    float normalize_length() { float len = length(); if (len) { *this /= len; } return len; }
    void clear() { x=0.0f; y=0.0f; z=0.0f; w=0.0f; }

    constexpr float dot(vec4 const& V) const { return x*V.x + y*V.y + z*V.z + w*V.w; }
    constexpr vec4 cross(vec4 const& V) const { return vec4(y*V.z - z*V.y, z*V.x - x*V.z, x*V.y - y*V.x, 0.0f); }

    constexpr vec3 to_vec3() const { return vec3(x, y, z); }
};

////////////////////////////////////////////////////////////////////////////////
// integer vector types

//------------------------------------------------------------------------------
class vec2i
{
public:
    int   x;
    int   y;

// constructors

    vec2i() = default;
    constexpr vec2i(int X, int Y) : x(X), y(Y) {}
    explicit constexpr vec2i(int S) : x(S), y(S) {}

    bool operator==(vec2i const& V) const { return x == V.x && y == V.y; }
    bool operator!=(vec2i const& V) const { return x != V.x || y != V.y; }
    constexpr int operator[](std::size_t idx) const { return (&x)[idx]; }
    int& operator[](std::size_t idx) { return (&x)[idx]; }
    operator int*() { return &x; }
    operator int const*() const { return &x; }

// algebraic vector operations

    constexpr vec2i operator-() const {return vec2i(-x, -y);}
    constexpr vec2i operator+(vec2i const& V) const { return vec2i(x+V.x, y+V.y); }
    constexpr vec2i operator-(vec2i const& V) const { return vec2i(x-V.x, y-V.y); }
    constexpr vec2i operator*(vec2i const& V) const { return vec2i(x*V.x, y*V.y); }
    constexpr vec2i operator/(vec2i const& V) const { return vec2i(x/V.x, y/V.y); }
    constexpr vec2i operator*(int S) const { return vec2i(x*S, y*S); }
    constexpr vec2i operator/(int S) const { return vec2i(x/S, y/S); }

// algebraic vector assignment operations

    vec2i& operator+=(vec2i const& V) { x+=V.x; y+=V.y; return *this; }
    vec2i& operator-=(vec2i const& V) { x-=V.x; y-=V.y; return *this; }
    vec2i& operator*=(vec2i const& V) { x*=V.x; y*=V.y; return *this; }
    vec2i& operator/=(vec2i const& V) { x/=V.x; y/=V.y; return *this; }
    vec2i& operator*=(int S) { x*=S; y*=S; return *this; }
    vec2i& operator/=(int S) { x/=S; y/=S; return *this; }

// utility functions

    double length() const { return std::sqrt(length_sqr()); }
    int length_sqr() const { return x*x + y*y; }
    void clear() { x=0; y=0; }

    constexpr int dot(vec2i const& V) const { return x*V.x + y*V.y; }
    constexpr vec2i cross(int V) const { return vec2i(y*V, -x*V); }
    explicit constexpr operator vec2() const { return vec2((float)x, (float)y); }
};

//------------------------------------------------------------------------------
inline vec2 rotate(vec2 v, float rad)
{
    float cosa = std::cos(rad);
    float sina = std::sin(rad);

    return vec2(v.x * cosa - v.y * sina,
                v.x * sina + v.y * cosa);
}

////////////////////////////////////////////////////////////////////////////////
// integer rectangle

//------------------------------------------------------------------------------
class rect
{
public:

// constructors

    rect() = default;
    constexpr rect(int X, int Y, int W, int H) : _mins(X, Y), _maxs(W, H) {}
    constexpr rect(vec2i mins, vec2i maxs) : _mins(mins), _maxs(maxs) {}

    bool operator==(rect const& R) const { return _mins == R._mins && _maxs == R._maxs; }
    bool operator!=(rect const& R) const { return _mins != R._mins || _maxs != R._maxs; }
    vec2i operator[](std::size_t idx) const { return (&_mins)[idx]; }
    vec2i& operator[](std::size_t idx) { return (&_mins)[idx]; }

    vec2i& mins() { return _mins; }
    vec2i& maxs() { return _mins; }
    vec2i mins() const { return _mins; }
    vec2i maxs() const { return _maxs; }

// algebraic vector operations

    rect operator+(vec2i const& V) const { return rect(_mins+V, _maxs+V); }
    rect operator-(vec2i const& V) const { return rect(_mins-V, _maxs-V); }
    rect operator*(int S) const { return rect(_mins*S, _maxs*S); }
    rect operator/(int S) const { return rect(_mins/S, _maxs/S); }

// boolean operations

    rect operator|(rect const& R) const {
        return rect(vec2i(std::min<int>(_mins.x, R._mins.x),
                          std::min<int>(_mins.y, R._mins.y)),
                    vec2i(std::max<int>(_maxs.x, R._maxs.x),
                          std::max<int>(_maxs.y, R._maxs.y)));
    }

    rect operator&(rect const& R) const {
        return rect(vec2i(std::max<int>(_mins.x, R._mins.x),
                          std::max<int>(_mins.y, R._mins.y)),
                    vec2i(std::min<int>(_maxs.x, R._maxs.x),
                          std::min<int>(_maxs.y, R._maxs.y)));
    }

// algebraic vector assignment operations

    rect& operator+=(vec2i const& V) { _mins += V; _maxs += V; return *this; }
    rect& operator-=(vec2i const& V) { _mins -= V; _maxs -= V; return *this; }
    rect& operator*=(int S) { _mins *= S; _maxs *= S; return *this; }
    rect& operator/=(int S) { _mins /= S; _maxs /= S; return *this; }

// boolean assignment operations

    rect& operator|=(rect const& R) { *this = *this | R; return *this; }
    rect& operator&=(rect const& R) { *this = *this & R; return *this; }

// utility functions

    vec2i center() const { return _mins + (_maxs - _mins) / 2; }
    vec2i size() const { return _maxs - _mins; }
    int area() const { return (_maxs.x - _mins.x) * (_maxs.y - _mins.y); }

    bool contains(vec2i point) const {
        return point.x >= _mins.x
            && point.y >= _mins.y
            && point.x <= _maxs.x
            && point.y <= _maxs.y;
    }

    void clear() { _mins.clear(); _maxs.clear(); }

    bool empty() const { return _maxs.x <= _mins.x || _maxs.y <= _mins.y; }
    bool inverted() const { return _maxs.x < _mins.x && _maxs.y < _mins.y; }

    static rect from_center(vec2i center, vec2i size) {
        return rect(center - size / 2, center + (size - size / 2));
    }

protected:
    vec2i _mins;
    vec2i _maxs;
};

//------------------------------------------------------------------------------
constexpr vec2 vec2_zero = vec2(0,0);
constexpr vec3 vec3_zero = vec3(0,0,0);
constexpr vec4 vec4_zero = vec4(0,0,0,0);

//------------------------------------------------------------------------------
template<> inline bool isnan<vec2>(vec2 v)
{
    return isnan(v[0]) || isnan(v[1]);
}

//------------------------------------------------------------------------------
template<> inline bool isnan<vec3>(vec3 v)
{
    return isnan(v[0]) || isnan(v[1]) || isnan(v[2]);
}

//------------------------------------------------------------------------------
template<> inline bool isnan<vec4>(vec4 v)
{
    return isnan(v[0]) || isnan(v[1]) || isnan(v[2]) || isnan(v[3]);
}
