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

    vec2 operator[](std::size_t idx) const { return _local[idx]; }
    std::size_t size() const { return _local.size(); }
    vec2 const* data() const { return _local.data(); }

    gl::vertex_buffer<vec2f> const& vertices() const { return _vertices; }
    std::size_t num_vertices() const { return _vertices.num_elements(); }

    gl::index_buffer<unsigned short> const& indices() const { return _indices; }
    std::size_t num_indices() const { return _indices.num_elements(); }

protected:
    std::vector<vec2> _local;

    gl::vertex_buffer<vec2f> _vertices;
    gl::index_buffer<unsigned short> _indices;
};

} // namespace render
