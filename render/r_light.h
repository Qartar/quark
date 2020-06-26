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
    light();

    void resize(vec2i size);
    void render(gl::framebuffer const& f) const;

protected:
    vec2i _size;
    vec2i _light_size;
    std::vector<gl::framebuffer> _framebuffers;
    gl::texture _texture;

    gl::shader _vertex_shader;

    gl::shader _mip_shader;
    gl::program _mip_program;

    gl::shader _light_shader;
    gl::program _light_program;

    gl::vertex_buffer<vec2> _vertex_buffer;
    gl::vertex_array _vertex_array;

protected:
    void generate_mips() const;
    void render_light() const;

    void check_shader(gl::shader const& s) const;
    void check_program(gl::program const& p) const;
};

} // namespace render
