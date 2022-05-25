// r_light.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_light.h"
#include "gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
light::light()
    : _size(0,0)
    , _light_size(0,0)
{
    _vertex_shader = gl::shader(gl::shader_stage::vertex, R"(
#version 330
layout(location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)");

    check_shader(_vertex_shader);

    _mip_shader = gl::shader(gl::shader_stage::fragment, R"(
#version 420
layout(binding = 0) uniform sampler2D lightmap;
void main() {
    ivec2 uv = ivec2(gl_FragCoord.xy) * 2;
    ivec2 sz = textureSize(lightmap, 0);

    float xmask = uv.x + 1 < sz.x ? 1.0 : 1.0;
    float ymask = uv.y + 1 < sz.y ? 1.0 : 1.0;
    vec4 mask = vec4(1.0, xmask, ymask, xmask * ymask);

    mat4 samples = mat4(
        texelFetch(lightmap, uv + ivec2(    0,     0), 0),
        texelFetch(lightmap, uv + ivec2(xmask,     0), 0),
        texelFetch(lightmap, uv + ivec2(    0, ymask), 0),
        texelFetch(lightmap, uv + ivec2(xmask, ymask), 0)
    );

    gl_FragColor = samples * mask;
}
)");

    check_shader(_mip_shader);

    _mip_program = gl::program(_vertex_shader, _mip_shader);

    check_program(_mip_program);

    _light_shader = gl::shader(gl::shader_stage::fragment, R"(
#version 420
layout(binding = 0) uniform sampler2D in_emissive;
layout(binding = 1) uniform sampler2D in_diffuse;

void main() {
    vec4 emissive = texelFetch(in_emissive, ivec2(gl_FragCoord.xy), 0);
    vec4 diffuse = texelFetch(in_diffuse, ivec2(gl_FragCoord.xy), 0);

    gl_FragColor = emissive + diffuse;
}
)");

    check_shader(_light_shader);

    _light_program = gl::program(_vertex_shader, _light_shader);

    check_program(_light_program);

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
void light::render(gl::framebuffer const& src, gl::framebuffer const& dst) const
{
    dst.draw();
    src.color_attachment(0).bind(0);
    src.color_attachment(1).bind(1);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    _light_program.use();
    _vertex_array.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);

    gl::program().use();
    gl::vertex_array().bind();
}

//------------------------------------------------------------------------------
void light::generate_mips() const
{
    _mip_program.use();

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
    _light_program.use();

    _texture.bind();
    // destination?

    glViewport(0, 0, _size.x, _size.y);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

//------------------------------------------------------------------------------
void light::check_shader(gl::shader const& s) const
{
    string::buffer info_log;

    if (s.compile_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }
}

//------------------------------------------------------------------------------
void light::check_program(gl::program const& p) const
{
    string::buffer info_log;

    if (p.link_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }

    if (p.validate_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }
}

} // namespace render
