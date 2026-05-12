// cm_bounds.h
//

#pragma once

#include "cm_vector.h"
#include "cm_matrix.h"

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
    bounds operator*(double S) const { return bounds(_mins*S, _maxs*S); }
    bounds operator/(double S) const { return bounds(_mins/S, _maxs/S); }

// boolean operations

    bounds operator|(bounds const& R) const {
        return bounds(vec2(std::min<double>(_mins.x, R._mins.x),
                           std::min<double>(_mins.y, R._mins.y)),
                      vec2(std::max<double>(_maxs.x, R._maxs.x),
                           std::max<double>(_maxs.y, R._maxs.y)));
    }

    bounds operator&(bounds const& R) const {
        return bounds(vec2(std::max<double>(_mins.x, R._mins.x),
                           std::max<double>(_mins.y, R._mins.y)),
                      vec2(std::min<double>(_maxs.x, R._maxs.x),
                           std::min<double>(_maxs.y, R._maxs.y)));
    }

// algebraic vector assignment operations

    bounds& operator+=(vec2 const& V) { _mins += V; _maxs += V; return *this; }
    bounds& operator-=(vec2 const& V) { _mins -= V; _maxs -= V; return *this; }
    bounds& operator*=(double S) { _mins *= S; _maxs *= S; return *this; }
    bounds& operator/=(double S) { _mins /= S; _maxs /= S; return *this; }

// boolean assignment operations

    bounds& operator|=(bounds const& R) { *this = *this | R; return *this; }
    bounds& operator&=(bounds const& R) { *this = *this & R; return *this; }

// utility functions

    vec2 center() const { return 0.5 * (_mins + _maxs); }
    vec2 size() const { return _maxs - _mins; }
    double area() const { return (_maxs.x - _mins.x) * (_maxs.y - _mins.y); }

    bounds& add(vec2 v) {
        _mins.x = std::min<double>(_mins.x, v.x);
        _mins.y = std::min<double>(_mins.y, v.y);
        _maxs.x = std::max<double>(_maxs.x, v.x);
        _maxs.y = std::max<double>(_maxs.y, v.y);
        return *this;
    }

    bounds expand(double s) const { return bounds(_mins - vec2(s), _maxs + vec2(s)); }
    bounds expand(vec2 v) const { return bounds(_mins - vec2(v), _maxs + vec2(v)); }

    bounds transform(mat3 tx) const {
        return from_points({
            vec2(_mins.x, _mins.y) * tx,
            vec2(_maxs.x, _mins.y) * tx,
            vec2(_mins.x, _maxs.y) * tx,
            vec2(_maxs.x, _maxs.y) * tx,
        });
    }

    bool contains(vec2 point) const {
        return point.x >= _mins.x
            && point.y >= _mins.y
            && point.x <= _maxs.x
            && point.y <= _maxs.y;
    }

    bool contains(bounds b) const {
        return _mins.x <= b._mins.x
            && _maxs.x >= b._maxs.x
            && _mins.y <= b._mins.y
            && _maxs.y >= b._maxs.y;
    }

    bool intersects(bounds b) const {
        return _mins.x <= b._maxs.x
            && _maxs.x >= b._mins.x
            && _mins.y <= b._maxs.y
            && _maxs.y >= b._mins.y;
    }

    bool intersects_circle(vec2 center, double radius) const {
        // Closest point on the bounds to the center of the circle
        vec2 point{
            center.x < _mins.x ? _mins.x : center.x > _maxs.x ? _maxs.x : center.x,
            center.y < _mins.y ? _mins.y : center.y > _maxs.y ? _maxs.y : center.y,
        };

        return (point - center).length_sqr() <= radius * radius;
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
        if (!num_points) {
            return {};
        }
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

//------------------------------------------------------------------------------
class bounds3
{
public:

// constructors

    bounds3() = default;
    constexpr bounds3(vec3 mins, vec3 maxs) : _mins(mins), _maxs(maxs) {}
    explicit bounds3(bounds const& b) : _mins(b[0]), _maxs(b[1]) {}

    bool operator==(bounds3 const& R) const { return _mins == R._mins && _maxs == R._maxs; }
    bool operator!=(bounds3 const& R) const { return _mins != R._mins || _maxs != R._maxs; }
    vec3 operator[](std::size_t idx) const { return (&_mins)[idx]; }
    vec3& operator[](std::size_t idx) { return (&_mins)[idx]; }

    vec3& mins() { return _mins; }
    vec3& maxs() { return _maxs; }
    vec3 mins() const { return _mins; }
    vec3 maxs() const { return _maxs; }

// algebraic vector operations

    bounds3 operator+(vec3 const& V) const { return bounds3(_mins+V, _maxs+V); }
    bounds3 operator-(vec3 const& V) const { return bounds3(_mins-V, _maxs-V); }
    bounds3 operator*(double S) const { return bounds3(_mins*S, _maxs*S); }
    bounds3 operator/(double S) const { return bounds3(_mins/S, _maxs/S); }

// boolean operations

    bounds3 operator|(bounds3 const& R) const {
        return bounds3(vec3(std::min<double>(_mins.x, R._mins.x),
                            std::min<double>(_mins.y, R._mins.y),
                            std::min<double>(_mins.z, R._mins.z)),
                       vec3(std::max<double>(_maxs.x, R._maxs.x),
                            std::max<double>(_maxs.y, R._maxs.y),
                            std::max<double>(_maxs.z, R._maxs.z)));
    }

    bounds3 operator&(bounds3 const& R) const {
        return bounds3(vec3(std::max<double>(_mins.x, R._mins.x),
                            std::max<double>(_mins.y, R._mins.y),
                            std::max<double>(_mins.z, R._mins.z)),
                       vec3(std::min<double>(_maxs.x, R._maxs.x),
                            std::min<double>(_maxs.y, R._maxs.y),
                            std::min<double>(_maxs.z, R._maxs.z)));
    }

// algebraic vector assignment operations

    bounds3& operator+=(vec3 const& V) { _mins += V; _maxs += V; return *this; }
    bounds3& operator-=(vec3 const& V) { _mins -= V; _maxs -= V; return *this; }
    bounds3& operator*=(double S) { _mins *= S; _maxs *= S; return *this; }
    bounds3& operator/=(double S) { _mins /= S; _maxs /= S; return *this; }

// boolean assignment operations

    bounds3& operator|=(bounds3 const& R) { *this = *this | R; return *this; }
    bounds3& operator&=(bounds3 const& R) { *this = *this & R; return *this; }

// utility functions

    vec3 center() const { return 0.5 * (_mins + _maxs); }
    vec3 size() const { return _maxs - _mins; }
    double volume() const { return (_maxs.x - _mins.x) * (_maxs.y - _mins.y) * (_maxs.z - _mins.z); }

    bounds3& add(vec3 v) {
        _mins.x = std::min<double>(_mins.x, v.x);
        _mins.y = std::min<double>(_mins.y, v.y);
        _mins.z = std::min<double>(_mins.z, v.z);
        _maxs.x = std::max<double>(_maxs.x, v.x);
        _maxs.y = std::max<double>(_maxs.y, v.y);
        _maxs.z = std::max<double>(_maxs.z, v.z);
        return *this;
    }

    bounds3 expand(double s) const { return bounds3(_mins - vec3(s), _maxs + vec3(s)); }
    bounds3 expand(vec3 v) const { return bounds3(_mins - vec3(v), _maxs + vec3(v)); }

    bounds3 transform(mat4 tx) const {
        return from_points({
            vec3(_mins.x, _mins.y, _mins.z) * tx,
            vec3(_maxs.x, _mins.y, _mins.z) * tx,
            vec3(_mins.x, _maxs.y, _mins.z) * tx,
            vec3(_maxs.x, _maxs.y, _mins.z) * tx,
            vec3(_mins.x, _mins.y, _maxs.z) * tx,
            vec3(_maxs.x, _mins.y, _maxs.z) * tx,
            vec3(_mins.x, _maxs.y, _maxs.z) * tx,
            vec3(_maxs.x, _maxs.y, _maxs.z) * tx,
        });
    }

    bool contains(vec3 point) const {
        return point.x >= _mins.x
            && point.y >= _mins.y
            && point.z >= _mins.z
            && point.x <= _maxs.x
            && point.y <= _maxs.y
            && point.z <= _maxs.z;
    }

    bool contains(bounds3 b) const {
        return _mins.x <= b._mins.x
            && _maxs.x >= b._maxs.x
            && _mins.y <= b._mins.y
            && _maxs.y >= b._maxs.y
            && _mins.z <= b._mins.z
            && _maxs.z >= b._maxs.z;
    }

    bool intersects(bounds3 b) const {
        return _mins.x <= b._maxs.x
            && _maxs.x >= b._mins.x
            && _mins.y <= b._maxs.y
            && _maxs.y >= b._mins.y
            && _mins.z <= b._maxs.z
            && _maxs.z >= b._mins.z;
    }

    bool intersects_circle(vec3 center, double radius) const {
        // Closest point on the bounds to the center of the circle
        vec3 point{
            center.x < _mins.x ? _mins.x : center.x > _maxs.x ? _maxs.x : center.x,
            center.y < _mins.y ? _mins.y : center.y > _maxs.y ? _maxs.y : center.y,
            center.z < _mins.z ? _mins.z : center.z > _maxs.z ? _maxs.z : center.z,
        };

        return (point - center).length_sqr() <= radius * radius;
    }

    void clear() { _mins.clear(); _maxs.clear(); }

    bool empty() const { return _maxs.x <= _mins.x || _maxs.y <= _mins.y || _maxs.z <= _mins.z; }
    bool inverted() const { return _maxs.x < _mins.x && _maxs.y < _mins.y && _maxs.z < _mins.z; }

    static bounds3 from_center(vec3 center, vec3 size) {
        return bounds3(center - size / 2, center + (size - size / 2));
    }

    static bounds3 from_translation(bounds3 b, vec3 t) {
        return b | (b + t);
    }

    static bounds3 from_points(vec3 const* points, std::size_t num_points) {
        if (!num_points) {
            return {};
        }
        bounds3 out = bounds3(points[0], points[0]);
        for (std::size_t ii = 1; ii < num_points; ++ii) {
            out.add(points[ii]);
        }
        return out;
    }

    template<std::size_t N> static bounds3 from_points(vec3 const (&points)[N]) {
        return from_points(points, N);
    }

protected:
    vec3 _mins;
    vec3 _maxs;
};
