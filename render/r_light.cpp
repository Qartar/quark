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
layout(binding = 0) uniform sampler2D lightmap;

vec2 image_to_world(ivec3 i) {
    return vec2(0.5) + vec2(i.xy * (1 << i.z));
}

vec4 illuminance(vec2 pos, ivec3 texel)
{
    vec2 light_pos = image_to_world(texel);
    vec2 dir = pos - light_pos;
    vec4 light = texelFetch(lightmap, texel.xy, texel.z);
    return light / dot(dir, dir);
}

const int lods = 13;

vec4 luminance(ivec2 pos)
{
    vec2 world_pos = image_to_world(ivec3(pos.xy, 0));

    vec4 total = vec4(0);
    vec4 prev = vec4(0);

    ivec3 texel = ivec3(0, 0, lods - 1);

    for (int ii = lods; ii > 0; --ii) {
        total -= prev;

        vec4 L00 = illuminance(world_pos, texel + ivec3(0,0,-1));
        vec4 L10 = illuminance(world_pos, texel + ivec3(1,0,-1));
        vec4 L01 = illuminance(world_pos, texel + ivec3(0,1,-1));
        vec4 L11 = illuminance(world_pos, texel + ivec3(1,1,-1));

        vec4 lum = vec4(0.2126, 0.7152, 0.0722, 0) * mat4(L00, L01, L10, L11);
        //vec4 lum = mat4(L00, L01, L10, L11) * vec4(0.2126, 0.7152, 0.0722, 0);
        float lmax = max(lum.x, max(lum.y, max(lum.z, lum.w)));

        total += L00 + L10 + L01 + L11;

        if (lmax == lum.x) {
            texel += ivec3(0,0,-1);
            prev = L00;
        } else if (lmax == lum.y) {
            texel += ivec3(0,1,-1);
            prev = L01;
        } else if (lmax == lum.z) {
            texel += ivec3(1,0,-1);
            prev = L10;
        } else {
            texel += ivec3(1,1,-1);
            prev = L11;
        }

        texel.xy *= 2;
    }

    return total;
}

void main() {
    gl_FragColor = 0.1 * luminance(ivec2(gl_FragCoord.xy));
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
