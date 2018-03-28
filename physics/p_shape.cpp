// p_shape.cpp
//

#include "p_shape.h"
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
convex_shape::convex_shape()
    : convex_shape({{1,0},{0,1},{-1,0},{0,-1}})
{}

//------------------------------------------------------------------------------
convex_shape::convex_shape(vec2 const* vertices, std::size_t num_vertices)
    : _num_vertices(std::min(num_vertices, kMaxVertices))
    , _area(0)
    , _center_of_mass(0,0)
{
    if (num_vertices > kMaxVertices) {
        constexpr std::size_t max_decimated = 1024;
        assert(num_vertices < max_decimated);
        vec2 decimated[max_decimated];

        // copy to temporary buffer
        std::copy(vertices, vertices + num_vertices, decimated);

        // extract convex hull in-place
        std::size_t num_decimated = _extract_convex_hull(decimated, num_vertices);

        // decimate in-place
        _num_vertices = _decimate_convex_hull(decimated, num_decimated, kMaxVertices);

        // copy to vertices
        std::copy(decimated, decimated + _num_vertices + 1, _vertices);
    } else {
        // copy to vertices
        std::copy(vertices, vertices + num_vertices, _vertices);

        // extract convex hull in-place
        _num_vertices = _extract_convex_hull(_vertices, num_vertices);
    }

    // calculate center of mass and area
    for (std::size_t ii = 0; ii < _num_vertices; ++ii) {
        float area = _vertices[ii].cross(_vertices[ii + 1]) * .5f;
        _center_of_mass += (_vertices[ii] + _vertices[ii + 1]) * area;
        _area += area;
    }
    _center_of_mass /= 3.f * _area;
}

//------------------------------------------------------------------------------
bool convex_shape::contains_point(vec2 point) const
{
    for (std::size_t ii = 0; ii < _num_vertices; ++ii) {
        vec2 va = point - _vertices[ii];
        vec2 vb = point - _vertices[ii + 1];
        if (va.cross(vb) < 0.f) {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
vec2 convex_shape::supporting_vertex(vec2 direction) const
{
    std::size_t imax = 0;
    float dmax = direction.dot(_vertices[0] - _center_of_mass);
    for (std::size_t ii = 1; ii < _num_vertices; ++ii) {
        float d = direction.dot(_vertices[ii] - _center_of_mass);
        if (d > dmax) {
            imax = ii;
            dmax = d;
        }
    }
    return _vertices[imax];
}

//------------------------------------------------------------------------------
void convex_shape::calculate_mass_properties(float inverse_mass, vec2& center_of_mass, float& inverse_inertia) const
{
    center_of_mass = _center_of_mass;
    if (inverse_mass > 0.0f) {
        float numerator = 0.f;
        float denominator = 0.f;
        for (std::size_t ii = 0; ii < _num_vertices; ++ii) {
            vec2 va = _vertices[ii] - _center_of_mass;
            vec2 vb = _vertices[ii + 1] - _center_of_mass;
            float cross = va.cross(vb);
            numerator += cross * (va.dot(va) + va.dot(vb) + vb.dot(vb));
            denominator += cross;
        }
        inverse_inertia = 6.f * inverse_mass * denominator / numerator;
    } else {
        inverse_inertia = 0.0f;
    }
}

//------------------------------------------------------------------------------
bounds convex_shape::calculate_bounds(vec2 position, float rotation) const
{
    mat3 transform = mat3::transform(position, rotation);
    vec2 initial = _vertices[0] * transform;
    bounds out(initial, initial);
    for (std::size_t ii = 1; ii < _num_vertices; ++ii) {
        out.add(_vertices[ii] * transform);
    }
    return out;
}

//------------------------------------------------------------------------------
std::size_t convex_shape::_extract_convex_hull(vec2* vertices, std::size_t num_vertices) const
{
    // find a point that is guaranteed to be on the convex hull
    vec2 pivot = vertices[0];
    for (std::size_t ii = 1; ii < num_vertices; ++ii) {
        if (vertices[ii].x > pivot.x || (vertices[ii].x == pivot.x && vertices[ii].y > pivot.y)) {
            pivot = vertices[ii];
        }
    }

    // radial sort about an arbitrary point on the convex hull
    std::sort(vertices, vertices + num_vertices, [pivot](vec2 a, vec2 b) {
        vec2 c = a - pivot;
        vec2 d = b - pivot;
        float s = c.y * d.x - d.y * c.x;

        if (c.y * d.y < 0.f) {
            return d.y < 0.f;
        } else if (s != 0.f) {
            return s < 0.f;
        } else {
            return c.length_sqr() < d.length_sqr();
        }
    });

    // graham scan to find vertices on the convex hull
    auto const is_convex = [](vec2 a, vec2 b, vec2 c) {
        return (c - b).cross(b - a) < 0.f;
    };

    std::size_t out = 3;
    for (std::size_t ii = 3; ii < num_vertices; ++ii, ++out) {
        while (out >= 2 && !is_convex(vertices[out - 2], vertices[out - 1], vertices[ii])) {
            --out;
        }
        vertices[out] = vertices[ii];
    }
    vertices[out] = vertices[0];
    return out;
}

//------------------------------------------------------------------------------
std::size_t convex_shape::_decimate_convex_hull(vec2* vertices, std::size_t num_vertices, std::size_t max_vertices) const
{
    // TODO!
    assert(num_vertices <= max_vertices);
    (void)vertices;
    (void)max_vertices;
    return num_vertices;
}

} // namespace physics
