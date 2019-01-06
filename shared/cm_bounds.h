// cm_bounds.h
//

#pragma once

#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
// axially aligned bounding boxes

//------------------------------------------------------------------------------
class bounds
{
public:

// constructors

    bounds() = default;
    constexpr bounds(vec2 mins, vec2 maxs) : _mins(mins), _maxs(maxs) {}

    bool operator==(bounds const& R) const { return _mins == R._mins && _maxs == R._maxs; }
    bool operator!=(bounds const& R) const { return _mins != R._mins || _maxs != R._maxs; }
    vec2 operator[](std::size_t idx) const { return (&_mins)[idx]; }
    vec2& operator[](std::size_t idx) { return (&_mins)[idx]; }

    vec2& mins() { return _mins; }
    vec2& maxs() { return _maxs; }
    vec2 mins() const { return _mins; }
    vec2 maxs() const { return _maxs; }

// algebraic vector operations

    bounds operator+(vec2 const& V) const { return bounds(_mins+V, _maxs+V); }
    bounds operator-(vec2 const& V) const { return bounds(_mins-V, _maxs-V); }
    bounds operator*(float S) const { return bounds(_mins*S, _maxs*S); }
    bounds operator/(float S) const { return bounds(_mins/S, _maxs/S); }

// boolean operations

    bounds operator|(bounds const& R) const {
        return bounds(vec2(std::min<float>(_mins.x, R._mins.x),
                           std::min<float>(_mins.y, R._mins.y)),
                      vec2(std::max<float>(_maxs.x, R._maxs.x),
                           std::max<float>(_maxs.y, R._maxs.y)));
    }

    bounds operator&(bounds const& R) const {
        return bounds(vec2(std::max<float>(_mins.x, R._mins.x),
                           std::max<float>(_mins.y, R._mins.y)),
                      vec2(std::min<float>(_maxs.x, R._maxs.x),
                           std::min<float>(_maxs.y, R._maxs.y)));
    }

// algebraic vector assignment operations

    bounds& operator+=(vec2 const& V) { _mins += V; _maxs += V; return *this; }
    bounds& operator-=(vec2 const& V) { _mins -= V; _maxs -= V; return *this; }
    bounds& operator*=(float S) { _mins *= S; _maxs *= S; return *this; }
    bounds& operator/=(float S) { _mins /= S; _maxs /= S; return *this; }

// boolean assignment operations

    bounds& operator|=(bounds const& R) { *this = *this | R; return *this; }
    bounds& operator&=(bounds const& R) { *this = *this & R; return *this; }

// utility functions

    vec2 center() const { return (_mins + _maxs) / 2.f; }
    vec2 size() const { return _maxs - _mins; }
    float area() const { return (_maxs.x - _mins.x) * (_maxs.y - _mins.y); }

    bounds& add(vec2 v) {
        _mins.x = std::min<float>(_mins.x, v.x);
        _mins.y = std::min<float>(_mins.y, v.y);
        _maxs.x = std::max<float>(_maxs.x, v.x);
        _maxs.y = std::max<float>(_maxs.y, v.y);
        return *this;
    }

    bounds expand(float s) const { return bounds(_mins - vec2(s), _maxs + vec2(s)); }
    bounds expand(vec2 v) const { return bounds(_mins - vec2(v), _maxs + vec2(v)); }

    bool contains(vec2 point) const {
        return point.x >= _mins.x
            && point.y >= _mins.y
            && point.x <= _maxs.x
            && point.y <= _maxs.y;
    }

    bool intersects(bounds b) const {
        return _mins.x <= b._maxs.x
            && _maxs.x >= b._mins.x
            && _mins.y <= b._maxs.y
            && _maxs.y >= b._mins.y;
    }

    void clear() { _mins.clear(); _maxs.clear(); }

    bool empty() const { return _maxs.x <= _mins.x || _maxs.y <= _mins.y; }
    bool inverted() const { return _maxs.x < _mins.x && _maxs.y < _mins.y; }

    static bounds from_center(vec2 center, vec2 size) {
        return bounds(center - size / 2, center + (size - size / 2));
    }

    static bounds from_translation(bounds b, vec2 t) {
        return b | (b + t);
    }

    static bounds from_points(vec2 const* points, std::size_t num_points) {
        bounds out = bounds(points[0], points[0]);
        for (std::size_t ii = 1; ii < num_points; ++ii) {
            out.add(points[ii]);
        }
        return out;
    }

    template<std::size_t N> static bounds from_points(vec2 const (&points)[N]) {
        return from_points(points, N);
    }

protected:
    vec2 _mins;
    vec2 _maxs;
};
