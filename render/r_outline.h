// r_outline.h
//

#pragma once

#include "cm_vector.h"

#include "gl/gl_buffer.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
class outline
{
public:
    outline() {}
    outline(vec2 const* vertices, std::size_t num_vertices);

    gl::vertex_buffer<vec2f> const& vertices() const { return _vertices; }
    std::size_t num_vertices() const { return _vertices.num_elements(); }

protected:
    gl::vertex_buffer<vec2f> _vertices;
};

} // namespace render
