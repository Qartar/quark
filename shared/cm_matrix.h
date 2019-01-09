// cm_matrix.h
//

#pragma once

#include "cm_vector.h"
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// matrix types

//------------------------------------------------------------------------------
class mat2
{
public:

// constructors

    mat2() = default;
    constexpr mat2(float in11, float in12,
                   float in21, float in22)
        : _rows{vec2{in11, in12},
                vec2{in21, in22}}
    {}
    constexpr mat2(vec2 r1, vec2 r2)
        : _rows{r1, r2}
    {}

    bool operator==(mat2 const& M) const { return _rows[0] == M[0] && _rows[1] == M[1]; }
    bool operator!=(mat2 const& M) const { return _rows[0] != M[0] || _rows[1] != M[1]; }
    constexpr vec2 operator[](std::size_t idx) const { return _rows[idx]; }
    vec2& operator[](std::size_t idx) { return _rows[idx]; }

// basic functions

    void set_identity() {
        _rows[0][0] = 1.f; _rows[0][1] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = 1.f;
    }

// rotation

    void set_rotation(float theta) {
        float cost = std::cos(theta);
        float sint = std::sin(theta);

        _rows[0][0] = +cost; _rows[0][1] = +sint;
        _rows[1][0] = -sint; _rows[1][1] = +cost;
    }

    static mat2 rotate(float theta) {
        float cost = std::cos(theta);
        float sint = std::sin(theta);

        return mat2(+cost, +sint, -sint, +cost);
    }

// scale

    void set_scale(vec2 const& s) {
        _rows[0][0] = s.x; _rows[0][1] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = s.y;
    }

    void set_scale(float s) { set_scale(vec2(s)); }

    constexpr static mat2 scale(float s) { return scale(vec2(s)); }
    constexpr static mat2 scale(float sx, float sy) { return scale(vec2(sx, sy)); }
    constexpr static mat2 scale(vec2 s) { return mat2(s.x, 0, 0, s.y); }

// multiplication

    constexpr friend vec2 operator*(vec2 const& v, mat2 const& m) {
        return vec2(v[0] * m[0][0] + v[1] * m[1][0],
                    v[0] * m[0][1] + v[1] * m[1][1]);
    }

    constexpr friend mat2 operator*(mat2 const& lhs, mat2 const& rhs) {
        return mat2(lhs[0] * rhs,
                    lhs[1] * rhs);
    }

protected:
    vec2 _rows[2];
};

//------------------------------------------------------------------------------
class mat3
{
public:

// constructors

    mat3() = default;
    constexpr mat3(float in11, float in12, float in13,
                   float in21, float in22, float in23,
                   float in31, float in32, float in33)
        : _rows{vec3{in11, in12, in13},
                vec3{in21, in22, in23},
                vec3{in31, in32, in33}}
    {}
    constexpr mat3(vec3 r1, vec3 r2, vec3 r3)
        : _rows{r1, r2, r3}
    {}

    bool operator==(mat3 const& M) const { return _rows[0] == M[0] && _rows[1] == M[1] && _rows[2] == M[2]; }
    bool operator!=(mat3 const& M) const { return _rows[0] != M[0] || _rows[1] != M[1] || _rows[2] != M[2]; }
    constexpr vec3 operator[](std::size_t idx) const { return _rows[idx]; }
    vec3& operator[](std::size_t idx) { return _rows[idx]; }

// basic functions

    void set_identity() {
        _rows[0][0] = 1.f; _rows[0][1] = 0.f; _rows[0][2] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = 1.f; _rows[1][2] = 0.f;
        _rows[2][0] = 0.f; _rows[2][1] = 0.f; _rows[2][2] = 1.f;
    }

// rotation

    template<int axis> void set_rotation(float theta) {
        float cost = std::cos(theta);
        float sint = std::sin(theta);

        constexpr int i0 = axis, i1 = (axis + 1) % 3, i2 = (axis + 2) % 3;

        _rows[i0][i0] = 1.f; _rows[i0][i1] =   0.f; _rows[i0][i2] =   0.f;
        _rows[i1][i0] = 0.f; _rows[i1][i1] = +cost; _rows[i1][i2] = +sint;
        _rows[i2][i0] = 0.f; _rows[i2][i1] = -sint; _rows[i2][i2] = +cost;
    }

    template<int axis> static mat3 rotate(float theta) {
        mat3 m; m.set_rotation<axis>(theta); return m;
    }

// scale

    void set_scale(vec3 const& s) {
        _rows[0][0] = s.x; _rows[0][1] = 0.f; _rows[0][2] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = s.y; _rows[1][2] = 0.f;
        _rows[2][0] = 0.f; _rows[2][1] = 0.f; _rows[2][2] = s.z;
    }

    void set_scale(float s) { set_scale(vec3(s)); }

    constexpr static mat3 scale(float s) { return scale(vec3(s)); }
    constexpr static mat3 scale(float sx, float sy, float sz) { return scale(vec3(sx, sy, sz)); }
    constexpr static mat3 scale(vec3 s) { return mat3(s.x, 0, 0, 0, s.y, 0, 0, 0, s.z); }

// homogenous transformation in two dimensions

    static mat3 transform(vec2 translation, float rotation) {
        float cosa = std::cos(rotation);
        float sina = std::sin(rotation);

        return mat3( cosa, sina, 0,
                    -sina, cosa, 0,
                    translation.x, translation.y, 1);
    }

// homogenous inverse transformation in two dimensions

    static mat3 inverse_transform(vec2 translation, float rotation) {
        float cosa = std::cos(rotation);
        float sina = std::sin(rotation);

        float tx = -translation.x * cosa - translation.y * sina;
        float ty =  translation.x * sina - translation.y * cosa;

        return mat3( cosa,-sina, 0,
                     sina, cosa, 0,
                     tx,   ty,   1);
    }

// multiplication

    constexpr friend vec2 operator*(vec2 const& v, mat3 const& m) {
        return vec2(v[0] * m[0][0] + v[1] * m[1][0] + m[2][0],
                    v[0] * m[0][1] + v[1] * m[1][1] + m[2][1]);
    }

    constexpr friend vec3 operator*(vec3 const& v, mat3 const& m) {
        return vec3(v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0],
                    v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1],
                    v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2]);
    }

    constexpr friend mat3 operator*(mat3 const& lhs, mat3 const& rhs) {
        return mat3(lhs[0] * rhs,
                    lhs[1] * rhs,
                    lhs[2] * rhs);
    }

protected:
    vec3 _rows[3];
};

//------------------------------------------------------------------------------
class mat4
{
public:

// constructors

    mat4() = default;
    constexpr mat4(float in11, float in12, float in13, float in14,
                   float in21, float in22, float in23, float in24,
                   float in31, float in32, float in33, float in34,
                   float in41, float in42, float in43, float in44)
        : _rows{vec4{in11, in12, in13, in14},
                vec4{in21, in22, in23, in24},
                vec4{in31, in32, in33, in34},
                vec4{in41, in42, in43, in44}}
    {}
    constexpr mat4(vec4 r1, vec4 r2, vec4 r3, vec4 r4)
        : _rows{r1, r2, r3, r4}
    {}

    bool operator==(mat4 const& M) const { return _rows[0] == M[0] && _rows[1] == M[1] && _rows[2] == M[2] && _rows[3] == M[3]; }
    bool operator!=(mat4 const& M) const { return _rows[0] != M[0] || _rows[1] != M[1] || _rows[2] != M[2] || _rows[3] != M[3]; }
    constexpr vec4 operator[](std::size_t idx) const { return _rows[idx]; }
    vec4& operator[](std::size_t idx) { return _rows[idx]; }

// basic functions

    void set_identity() {
        _rows[0][0] = 1.f; _rows[0][1] = 0.f; _rows[0][2] = 0.f; _rows[0][3] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = 1.f; _rows[1][2] = 0.f; _rows[1][3] = 0.f;
        _rows[2][0] = 0.f; _rows[2][1] = 0.f; _rows[2][2] = 1.f; _rows[2][3] = 0.f;
        _rows[3][0] = 0.f; _rows[3][1] = 0.f; _rows[3][2] = 0.f; _rows[3][3] = 1.f;
    }

// rotation

    template<int axis> void set_rotation(float theta) {
        float cost = std::cos(theta);
        float sint = std::sin(theta);

        constexpr int i0 = axis, i1 = (axis + 1) % 3, i2 = (axis + 2) % 3;

        _rows[i0][i0] = 1.f; _rows[i0][i1] =   0.f; _rows[i0][i2] =   0.f; _rows[ 0][ 3] = 0.f;
        _rows[i1][i0] = 0.f; _rows[i1][i1] = +cost; _rows[i1][i2] = +sint; _rows[ 1][ 3] = 0.f;
        _rows[i2][i0] = 0.f; _rows[i2][i1] = -sint; _rows[i2][i2] = +cost; _rows[ 2][ 3] = 0.f;
        _rows[ 3][ 0] = 0.f; _rows[ 3][ 1] =   0.f; _rows[ 3][ 2] =   0.f; _rows[ 3][ 3] = 1.f;
    }

    template<int axis> static mat4 rotate(float theta) {
        mat4 m; m.set_rotation<axis>(theta); return m;
    }

// translation

    void set_translation(vec4 const& t) {
        _rows[0][0] = 1.f; _rows[0][1] = 0.f; _rows[0][2] = 0.f; _rows[0][3] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = 1.f; _rows[1][2] = 0.f; _rows[1][3] = 0.f;
        _rows[2][0] = 0.f; _rows[2][1] = 0.f; _rows[2][2] = 1.f; _rows[2][3] = 0.f;
        _rows[3][0] = t.x; _rows[3][1] = t.y; _rows[3][2] = t.z; _rows[3][3] = t.w;
    }

    void set_translation(vec3 const& t) { set_translation(vec4(t)); }

    constexpr static mat4 translate(float tx, float ty, float tz) { return translate(vec3(tx, ty, tz)); }
    constexpr static mat4 translate(vec3 t) { return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t.x, t.y, t.z, 1); }

// scale

    void set_scale(vec4 const& s) {
        _rows[0][0] = s.x; _rows[0][1] = 0.f; _rows[0][2] = 0.f; _rows[0][3] = 0.f;
        _rows[1][0] = 0.f; _rows[1][1] = s.y; _rows[1][2] = 0.f; _rows[1][3] = 0.f;
        _rows[2][0] = 0.f; _rows[2][1] = 0.f; _rows[2][2] = s.z; _rows[2][3] = 0.f;
        _rows[3][0] = 0.f; _rows[3][1] = 0.f; _rows[3][2] = 0.f; _rows[3][3] = s.w;
    }

    void set_scale(float s) { set_scale(vec4(s)); }

    constexpr static mat4 scale(float s) { return scale(vec4(s)); }
    constexpr static mat4 scale(float sx, float sy, float sz, float sw = 1) { return scale(vec4(sx, sy, sz, sw)); }
    constexpr static mat4 scale(vec4 s) { return mat4(s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0, 0, 0, 0, s.w); }

// multiplication

    constexpr friend vec3 operator*(vec3 const& v, mat4 const& m) {
        return vec3(v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + m[3][0],
                    v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + m[3][1],
                    v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + m[3][2]);
    }

    constexpr friend vec4 operator*(vec4 const& v, mat4 const& m) {
        return vec4(v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0],
                    v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1],
                    v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2],
                    v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3]);
    }

    constexpr friend mat4 operator*(mat4 const& lhs, mat4 const& rhs) {
        return mat4(lhs[0] * rhs,
                    lhs[1] * rhs,
                    lhs[2] * rhs,
                    lhs[3] * rhs);
    }

protected:
    vec4 _rows[4];
};

//------------------------------------------------------------------------------
constexpr mat2 mat2_identity = mat2(1,0,0,1);
constexpr mat3 mat3_identity = mat3(1,0,0,0,1,0,0,0,1);
constexpr mat4 mat4_identity = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);

//------------------------------------------------------------------------------
template<> inline bool isnan<mat2>(mat2 m)
{
    return isnan(m[0]) || isnan(m[1]);
}

//------------------------------------------------------------------------------
template<> inline bool isnan<mat3>(mat3 m)
{
    return isnan(m[0]) || isnan(m[1]) || isnan(m[2]);
}

//------------------------------------------------------------------------------
template<> inline bool isnan<mat4>(mat4 m)
{
    return isnan(m[0]) || isnan(m[1]) || isnan(m[2]) || isnan(m[3]);
}
