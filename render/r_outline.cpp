// r_outline.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_outline.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
outline::outline(vec2 const* vertices, std::size_t num_vertices)
{
    std::vector<vec2f> s(num_vertices);
    for (std::size_t ii = 0; ii < num_vertices; ++ii) {
        s[ii] = vec2f(vertices[ii]);
    }
    _vertices = gl::vertex_buffer<vec2f>(
        gl::buffer_usage::static_,
        gl::buffer_access::draw,
        s.size(),
        s.data());
}

} // namespace render
