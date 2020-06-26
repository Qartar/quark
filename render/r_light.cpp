// r_light.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_light.h"
#include "r_shader.h"
#include "gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
light::light(render::system* renderer)
    : _size(0,0)
    , _light_size(0,0)
    , _mip_shader(renderer->load_shader("mip", "assets/shader/light_v.glsl", "assets/shader/light_f_mip.glsl"))
    , _light_shader(renderer->load_shader("light", "assets/shader/light_v.glsl", "assets/shader/light_f.glsl"))
{
    _vertex_buffer = gl::vertex_buffer<vec2>(
        gl::buffer_usage::static_,
        gl::buffer_access::draw,
        {{-3, 1}, {1, 1}, {1, -3}});

    _vertex_array = gl::vertex_array({
        {2, GL_FLOAT, gl::vertex_attrib_type::float_, 0}
    });

    _vertex_array.bind_buffer(_vertex_buffer, 0);

    gl::vertex_array().bind();
    gl::vertex_buffer<vec2>().bind();
}

//------------------------------------------------------------------------------
void light::resize(vec2i size)
{
    if (_size != size) {
        _size = size;
        _light_size = vec2i(1,1);
        while (_light_size.x < _size.x) {
            _light_size *= 2;
        }

        int levels = 1 + std::ilogb(max(_light_size.x, _light_size.y));
        _texture = gl::texture2d(levels, GL_RGBA16F, _light_size.x, _light_size.y);
        _framebuffers.resize(0);
        // Create a separate framebuffer for each texture level
        for (int ii = 0; ii < levels; ++ii) {
            _framebuffers.push_back(gl::framebuffer(max(1, _light_size.x >> ii), max(1, _light_size.y >> ii), {
                {gl::attachment_type::color, GL_RGBA16F, &_texture, ii}
            }));
        }
    }
}

//------------------------------------------------------------------------------
void light::render(gl::framebuffer const& f) const
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    _framebuffers[0].blit(f,
        0, 0, _size.x, _size.y,
        0, 0, _size.x, _size.y,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    _vertex_array.bind();

    generate_mips();
#if 1
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    f.draw();

    render_light();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
    //  2560    1280     640     320     160      80      40      20      10       5       (2)      1
    //  1440     720     360     180      90      45     (22)     11      (5)     (2)       1       1

    static int index = 0;
    int level = 1 + (++index / 100) % (_framebuffers.size() - 1);
    f.blit(_framebuffers[level],
        0, 0, max(1, _light_size.x >> level), max(1, _light_size.y >> level),
        0, 0, _light_size.x >> 1, _light_size.y >> 1,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

#endif

    gl::program().use();
    gl::vertex_array().bind();

    f.draw();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

//------------------------------------------------------------------------------
void light::generate_mips() const
{
    _mip_shader->program().use();

    for (int level = 1; level < _texture.levels(); ++level) {
        _framebuffers[level - 1].color_attachment().bind();
        _framebuffers[level - 0].draw();
        glViewport(0, 0, max(1, _light_size.x >> level), max(1, _light_size.y >> level));
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

//------------------------------------------------------------------------------
void light::render_light() const
{
    _light_shader->program().use();

    _texture.bind();
    // destination?

    glViewport(0, 0, _size.x, _size.y);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

} // namespace render
