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
    glBlendColor = (PFNGLBLENDCOLOR )wglGetProcAddress("glBlendColor");

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
layout(binding = 2) uniform sampler2D in_lighting;

void main() {
    vec4 emissive = texelFetch(in_emissive, ivec2(gl_FragCoord.xy), 0);
    vec4 diffuse = texelFetch(in_diffuse, ivec2(gl_FragCoord.xy), 0);
    vec4 lighting = texelFetch(in_lighting, ivec2(gl_FragCoord.xy), 0);

    gl_FragColor = emissive + diffuse * lighting + diffuse * 0.5;
}
)");

    check_shader(_light_shader);

    _light_program = gl::program(_vertex_shader, _light_shader);

    check_program(_light_program);

    _upsample_shader = gl::shader(gl::shader_stage::fragment, R"(
#version 430

//  input
layout(binding = 0) uniform sampler2D inColor;
layout(location = 0) uniform float kernelRadius;

//  output
layout(location = 0) out vec4 outColor;

//------------------------------------------------------------------------------
void main()
{
    // texel size in the input image
    vec2 texelSize = vec2(1.0) / vec2(textureSize(inColor, 0));
    // texel position in the input image
    vec2 texelCoord = 0.5 * gl_FragCoord.xy * texelSize;
    // kernel size in texels of the input image
    vec2 kernelSize = kernelRadius * texelSize / texelSize.yy;

    //
    //  Perform 9-tap filter as described in Jimenez(2014)
    //

    const vec2 kernelOffset[9] = {
        vec2(-1., -1.), vec2( 0., -1.), vec2( 1., -1.), //  0  1  2
        vec2(-1.,  0.), vec2( 0.,  0.), vec2( 1.,  0.), //  3  4  5
        vec2(-1.,  1.), vec2( 0.,  1.), vec2( 1.,  1.)  //  6  7  8
    };

    vec4 tap[9];

    // Fetch bilinear samples
    for (int ii = 0; ii < 9; ++ii) {
        vec2 pos = texelCoord + kernelOffset[ii] * kernelSize;
        tap[ii] = textureLod(inColor, pos, 0);
    }

    // Weight and sum
    outColor = (1. / 16.) * tap[0] + (2. / 16.) * tap[1] + (1. / 16.) * tap[2]
             + (2. / 16.) * tap[3] + (4. / 16.) * tap[4] + (2. / 16.) * tap[5]
             + (1. / 16.) * tap[6] + (2. / 16.) * tap[7] + (1. / 16.) * tap[8];
}
)");

    check_shader(_upsample_shader);

    _upsample_program = gl::program(_vertex_shader, _upsample_shader);

    check_program(_upsample_program);

    _downsample_shader = gl::shader(gl::shader_stage::fragment, R"(
#version 430

//  input
layout(binding = 0) uniform sampler2D inColor;
layout(location = 0) uniform float base_luma;

//  output
layout(location = 0) out vec4 outColor;

//------------------------------------------------------------------------------
vec4 box_filter_luma(in vec4 a, in vec4 b, in vec4 c, in vec4 d)
{
    // Conversion factors from RGB to luma
    const vec4 luma = vec4(0.2126, 0.7152, 0.0722, 0.);
    // Place input colors into matrix columns
    mat4 color = mat4(a, b, c, d);
    // Calculate weights for each column (color)
    vec4 weight = vec4(base_luma) / (vec4(base_luma) + luma * color);
    // Apply weights to each column and return weighted average
    return color * weight / dot(vec4(1.), weight);
}

//------------------------------------------------------------------------------
vec4 box_filter(in vec4 a, in vec4 b, in vec4 c, in vec4 d)
{
    // Place input colors into matrix columns
    mat4 color = mat4(a, b, c, d);
    // Return weighted average
    return color * vec4(0.25);
}

//------------------------------------------------------------------------------
void main()
{
    // texel size in the input image
    vec2 texelSize = vec2(1.0) / vec2(textureSize(inColor, 0));
    // texel position in the input image
    vec2 texelCoord = 2.0 * gl_FragCoord.xy * texelSize;

    //
    //  Perform 13-tap filter as described in Jimenez(2014)
    //

    const vec2 texelOffset[13] = {
        vec2(-2., -2.), vec2( 0., -2.), vec2( 2., -2.), //  0   1   2
                vec2(-1., -1.), vec2( 1., -1.),         //    3   4
        vec2(-2.,  0.), vec2( 0.,  0.), vec2( 2.,  0.), //  5   6   7
                vec2(-1.,  1.), vec2( 1.,  1.),         //    8   9
        vec2(-2.,  2.), vec2( 0.,  2.), vec2( 2.,  2.)  // 10  11  12
    };

    vec4 tap[13];

    // Fetch bilinear samples
    for (int ii = 0; ii < 13; ++ii) {
        vec2 pos = texelCoord + texelOffset[ii] * texelSize;
        tap[ii] = textureLod(inColor, pos, 0);
    }

    // Weight and sum
    if (base_luma > 0.) {
        outColor = 0.125 * box_filter_luma(tap[ 0], tap[ 1], tap[ 5], tap[ 6])
                 + 0.125 * box_filter_luma(tap[ 1], tap[ 2], tap[ 6], tap[ 7])
                 + 0.500 * box_filter_luma(tap[ 3], tap[ 4], tap[ 8], tap[ 9])
                 + 0.125 * box_filter_luma(tap[ 5], tap[ 6], tap[10], tap[11])
                 + 0.125 * box_filter_luma(tap[ 6], tap[ 7], tap[11], tap[12]);
    } else {
        outColor = 0.125 * box_filter(tap[ 0], tap[ 1], tap[ 5], tap[ 6])
                 + 0.125 * box_filter(tap[ 1], tap[ 2], tap[ 6], tap[ 7])
                 + 0.500 * box_filter(tap[ 3], tap[ 4], tap[ 8], tap[ 9])
                 + 0.125 * box_filter(tap[ 5], tap[ 6], tap[10], tap[11])
                 + 0.125 * box_filter(tap[ 6], tap[ 7], tap[11], tap[12]);
    }
}
)");

    check_shader(_downsample_shader);

    _downsample_program = gl::program(_vertex_shader, _downsample_shader);

    check_program(_downsample_program);

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
        _light_size = _size;

        int levels = 8;
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
    config::integer r_showBloomBuffer("r_showBloomBuffer", 0, 0, "");
    config::scalar r_bloomAlpha("r_bloomAlpha", 0.5, 0, "");

    _vertex_array.bind();

    //
    // downsample
    //

    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    _framebuffers[0].blit(src, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    _downsample_program.use();

    for (int level = 1; level < _texture.levels(); ++level) {
        _framebuffers[level - 1].color_attachment().bind();
        _framebuffers[level].draw();
        glViewport(0, 0, max(1, _light_size.x >> level), max(1, _light_size.y >> level));
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    //
    // upsample
    //

    if (r_showBloomBuffer >= 0) {

        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, r_bloomAlpha);
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);

        _upsample_program.use();

        for (int level = _texture.levels() - 2; level >= 0; --level) {
            _framebuffers[level].draw();
            _framebuffers[level + 1].color_attachment().bind();

            _upsample_program.uniform(0, 0.2f / (1 << (_texture.levels() - level - 1)));

            glViewport(0, 0,
                max(1, _light_size.x >> level),
                max(1, _light_size.y >> level));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    //
    // resolve
    //

    if (r_showBloomBuffer) {
        int buffer = clamp(abs(r_showBloomBuffer) - 1, 0, _texture.levels() - 1);
        dst.blit(_framebuffers[buffer], GL_COLOR_BUFFER_BIT, GL_LINEAR);

        dst.draw();
        glViewport(0, 0, _size.x, _size.y);
    } else {
        dst.draw();
        src.color_attachment(0).bind(0);
        src.color_attachment(1).bind(1);
        _framebuffers[0].color_attachment().bind(2);

        glDisable(GL_BLEND);

        _light_program.use();
        glViewport(0, 0, _size.x, _size.y);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    gl::program().use();
    gl::vertex_array().bind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
