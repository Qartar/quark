// r_model.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_model.h"

render::model constellation({
    {-2.f + -10.5f,   6.f},
    {-2.f +  -8.5f,  12.f},
    {-2.f +  -7.f,   14.f},
    {-2.f +   0.f,   14.f},
    {-2.f +   3.5f,  13.f},
    {-2.f +   3.5f,  11.f},
    {-2.f +   0.f,   11.f},
    {-2.f +   0.f,    7.f},
    {-2.f +   3.5f,   7.f},
    {-2.f +  10.5f,   5.f},
    {-2.f +  10.5f,   3.5f},
    {-2.f +  14.f,    3.5f},
    {-2.f +  14.f,    7.f},
    {-2.f +  17.5f,   7.f},
    {-2.f +  19.5f,   6.f},
    {-2.f +  21.f,    2.f},
    {-2.f +  21.f,   -2.f},
    {-2.f +  19.5f,  -6.f},
    {-2.f +  17.5f,  -7.f},
    {-2.f +  14.f,   -7.f},
    {-2.f +  14.f,   -3.5f},
    {-2.f +  10.5f,  -3.5f},
    {-2.f +  10.5f,  -5.f},
    {-2.f +   3.5f,  -7.f},
    {-2.f +   0.f,   -7.f},
    {-2.f +   0.f,  -11.f},
    {-2.f +   3.5f, -11.f},
    {-2.f +   3.5f, -13.f},
    {-2.f +   0.f,  -14.f},
    {-2.f +  -7.f,  -14.f},
    {-2.f +  -8.5f, -12.f},
    {-2.f + -10.5f,  -6.f},
},
{
    { 0,  1,  7, .5f},
    { 1,  6,  7, .5f},
    { 1,  2,  6, .5f},
    { 2,  3,  6, .5f},
    { 3,  4,  5, .5f},
    { 3,  5,  6, .5f},
    { 0,  7, 24, .5f},
    { 0, 24, 31, .5f},
    { 7,  8, 23, .5f},
    { 7, 23, 24, .5f},
    { 8, 10, 21, .5f},
    { 8, 21, 23, .5f},
    { 8,  9, 10, .5f},
    {21, 22, 23, .5f},
    {10, 11, 20, .5f},
    {10, 20, 21, .5f},
    {11, 12, 13, .5f},
    {11, 13, 14, .5f},
    {11, 14, 15, .5f},
    {11, 15, 16, .5f},
    {11, 16, 20, .5f},
    {20, 19, 18, .5f},
    {20, 18, 17, .5f},
    {20, 17, 16, .5f},
    {31, 30, 24, .5f},
    {30, 25, 24, .5f},
    {30, 29, 25, .5f},
    {29, 28, 25, .5f},
    {28, 27, 26, .5f},
    {28, 26, 25, .5f},
});

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
model::model(model::rect const* rects, std::size_t num_rects)
{
    for (std::size_t ii = 0; ii < num_rects; ++ii) {
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 0));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 1));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 2));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 1));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 3));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 2));

        vec2 mins = rects[ii].center - rects[ii].size * 0.5f;
        vec2 maxs = rects[ii].center + rects[ii].size * 0.5f;

        _vertices.emplace_back(mins.x, mins.y);
        _vertices.emplace_back(maxs.x, mins.y);
        _vertices.emplace_back(mins.x, maxs.y);
        _vertices.emplace_back(maxs.x, maxs.y);

        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
    }

    _bounds = bounds::from_points(_vertices.data(), _vertices.size());
}

//------------------------------------------------------------------------------
model::model(vec2 const* vertices, std::size_t num_vertices, triangle const* triangles, std::size_t num_triangles)
{
    for (std::size_t ii = 0; ii < num_triangles; ++ii) {
        _indices.push_back(static_cast<uint16_t>(_vertices.size() + 0));
        _indices.push_back(static_cast<uint16_t>(_vertices.size() + 1));
        _indices.push_back(static_cast<uint16_t>(_vertices.size() + 2));

        assert(triangles[ii].v0 < num_vertices);
        assert(triangles[ii].v1 < num_vertices);
        assert(triangles[ii].v2 < num_vertices);

        _vertices.push_back(vertices[triangles[ii].v0]);
        _vertices.push_back(vertices[triangles[ii].v1]);
        _vertices.push_back(vertices[triangles[ii].v2]);

        _colors.emplace_back(triangles[ii].gamma, triangles[ii].gamma, triangles[ii].gamma);
        _colors.emplace_back(triangles[ii].gamma, triangles[ii].gamma, triangles[ii].gamma);
        _colors.emplace_back(triangles[ii].gamma, triangles[ii].gamma, triangles[ii].gamma);
    }

    _bounds = bounds::from_points(_vertices.data(), _vertices.size());
}

//------------------------------------------------------------------------------
bool model::contains(vec2 point) const
{
    if (!_bounds.contains(point)) {
        return false;
    }

    for (std::size_t ii = 0; ii < _indices.size(); ii += 3) {
        uint16_t i0 = _indices[ii + 0];
        uint16_t i1 = _indices[ii + 1];
        uint16_t i2 = _indices[ii + 2];

        vec2 v0 = point - _vertices[i0];
        vec2 v1 = point - _vertices[i1];
        vec2 v2 = point - _vertices[i2];

        vec2 e0 = _vertices[i1] - _vertices[i0];
        vec2 e1 = _vertices[i2] - _vertices[i1];
        vec2 e2 = _vertices[i0] - _vertices[i2];

        float s0 = v0.cross(e0);
        float s1 = v1.cross(e1);
        float s2 = v2.cross(e2);

        if (s0 * s1 > 0.f && s1 * s2 > 0.f) {
            return true;
        }
    }

    return false;
}

} // namespace render
