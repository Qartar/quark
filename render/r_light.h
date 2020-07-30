// r_light.h
//

#pragma once

#include "cm_vector.h"

#include "gl/gl_buffer.h"
#include "gl/gl_framebuffer.h"
#include "gl/gl_shader.h"
#include "gl/gl_texture.h"
#include "gl/gl_vertex_array.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

class light
{
public:
    light(render::system* renderer);

    void resize(vec2i size);
    void render(gl::framebuffer const& f) const;

protected:
    vec2i _size;
    vec2i _light_size;
    std::vector<gl::framebuffer> _framebuffers;

    gl::texture _lightmap;
    gl::texture _barycenter;

    render::shader const* _mip_shader;
    render::shader const* _mip0_shader;
    render::shader const* _light_shader;

    gl::vertex_buffer<vec2> _vertex_buffer;
    gl::vertex_array _vertex_array;

protected:
    void generate_mips() const;
    void render_light() const;
};

} // namespace render
