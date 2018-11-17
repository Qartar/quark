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

//------------------------------------------------------------------------------
convex_shape convex_shape::from_planes(vec3 const* planes, std::size_t num_planes)
{
    constexpr std::size_t max_enumerated = 8192;
    vec2 enumerated[max_enumerated];
    std::size_t num_enumerated = 0;

    for (std::size_t ii = 0; ii < num_planes; ++ii) {
        vec2 v = planes[ii].to_vec2().cross(1.f);
        vec2 vmin = v * -1e+30f;
        vec2 vmax = v * +1e+30f;

        for (std::size_t jj = 0; jj < num_planes; ++jj) {
            vec3 p = planes[ii].cross(planes[jj]);
            if (p.z) {
                vec2 u = planes[jj].to_vec2();
                vec2 x = vec2(p.x, p.y) / p.z;
                float s = u.dot(v);
                if (s < 0.f && u.dot(vmin - x) > 0.f) {
                    vmin = x;
                }
                if (s > 0.f && u.dot(vmax - x) > 0.f) {
                    vmax = x;
                }
                if (v.dot(vmax - vmin) < 0.f) {
                    break;
                }
            }
        }
        float d = v.dot(vmax - vmin);
        if (d > 0.f) {
            enumerated[num_enumerated++] = vmin;
            enumerated[num_enumerated++] = vmax;
        } else if (d == 0.f) {
            enumerated[num_enumerated++] = vmin;
        }
    }

    return convex_shape(enumerated, num_enumerated);
}

//------------------------------------------------------------------------------
convex_shape convex_shape::shrink_by_radius(float radius) const
{
    vec3 planes[kMaxVertices];

    // in order to robustly handle self-penetration we calculate the edge planes
    // after shrinking and rebuild the convex hull via vertex enumeration.
    for (std::size_t ii = 0; ii < _num_vertices; ++ii) {
        vec2 n = (_vertices[ii + 1] - _vertices[ii]).cross(1.f);
        planes[ii] = vec3(n.x, n.y, radius * n.length() - n.dot(_vertices[ii]));
    }

    return from_planes(planes, _num_vertices);
}

//------------------------------------------------------------------------------
convex_shape convex_shape::expand_by_radius(float radius) const
{
    vec2 normals[kMaxVertices + 1];

    normals[0] = (_vertices[0] - _vertices[_num_vertices - 1]).cross(1.f).normalize();
    for (std::size_t ii = 1; ii < _num_vertices; ++ii) {
        normals[ii] = (_vertices[ii] - _vertices[ii - 1]).cross(1.f).normalize();
        assert(normals[ii] != normals[ii - 1] && "non-convex (collinear) faces detected");
    }
    normals[_num_vertices] = normals[0];

    // replace each vertex with the intersection of the translated edges
    convex_shape out = *this;

    for (std::size_t ii = 0; ii < _num_vertices; ++ii) {
        vec2 n1 = normals[ii];
        vec2 n2 = normals[ii + 1];

        vec3 u = vec3(n1.x, n1.y, -radius - n1.dot(_vertices[ii]));
        vec3 v = vec3(n2.x, n2.y, -radius - n2.dot(_vertices[ii]));
        vec3 p = u.cross(v);

        out._vertices[ii] = vec2(p.x, p.y) / p.z;
    }
    out._vertices[_num_vertices] = out._vertices[0];

    // re-calculate center of mass and area
    out._area = 0;
    out._center_of_mass = vec2(0,0);
    for (std::size_t ii = 0; ii < out._num_vertices; ++ii) {
        float cross = out._vertices[ii].cross(out._vertices[ii + 1]) * .5f;
        out._center_of_mass += (out._vertices[ii] + out._vertices[ii + 1]) * cross;
        out._area += cross;
    }
    out._center_of_mass /= 3.f * out._area;

    return out;
}

} // namespace physics
