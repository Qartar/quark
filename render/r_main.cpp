// r_main.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "gl/gl_include.h"
#include "gl/gl_buffer.h"
#include "gl/gl_framebuffer.h"
#include "gl/gl_shader.h"
#include "gl/gl_texture.h"
#include "gl/gl_vertex_array.h"

#include "r_font.h"
#include "r_image.h"
#include "r_shader.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
system::system(render::window* window)
    : _command_list_shaders("listShaders", this, &system::command_list_shaders)
    , _command_reload_shaders("reloadShaders", this, &system::command_reload_shaders)
    , _framebuffer_width("r_width", 0, config::archive, "framebuffer width, or 0 to use window width")
    , _framebuffer_height("r_height", 0, config::archive, "framebuffer height, or 0 to use window height")
    , _framebuffer_scale("r_scale", 1, config::archive, "framebuffer scale if using window dimensions")
    , _framebuffer_samples("r_samples", -1, config::archive, "framebuffer samples, or -1 to use maximum supported")
    , _bloom("r_bloom", true, config::archive, "")
    , _window(window)
    , _view{}
    , _view_bounds{}
    , _draw_tris("r_tris", 0, 0, "draw triangle edges")
    , _graph("r_graph", false, 0, "draw frame timing graph")
{}

//------------------------------------------------------------------------------
system::~system()
{
}

//------------------------------------------------------------------------------
result system::init()
{
    random r;

    glBlendColor = (PFNGLBLENDCOLOR )wglGetProcAddress("glBlendColor");

    gl::buffer::init();
    gl::framebuffer::init();
    gl::shader::init();
    gl::program::init();
    gl::texture::init();
    gl::vertex_array::init();

    font::init();

    _bloom_downsample = load_shader("bloom_downsample", "assets/shader/bloom.vsh", "assets/shader/bloom_downsample.fsh");
    _bloom_upsample = load_shader("bloom_upsample", "assets/shader/bloom.vsh", "assets/shader/bloom_upsample.fsh");

    _bloom_ibo = gl::index_buffer<uint16_t>(gl::buffer_usage::static_, gl::buffer_access::draw,
        {0, 1, 2}
    );
    _bloom_vbo = gl::vertex_buffer<vec2>(gl::buffer_usage::static_, gl::buffer_access::draw,
        {vec2(-1,-1), vec2(3,-1), vec2(-1,3)}
    );
    _bloom_vao = gl::vertex_array({
        gl::vertex_array_attrib{2, GL_FLOAT, gl::vertex_attrib_type::float_, 0}
    });
    _bloom_vao.bind_buffer(_bloom_ibo);
    _bloom_vao.bind_buffer(_bloom_vbo, 0);

    gl::index_buffer<int>().bind();
    gl::vertex_buffer<int>().bind();

    _view.size = vec2(_window->size());
    _view.origin = _view.size * 0.5f;
    _view.viewport = {};

    resize(_window->size());

    _starfield_points.resize(2048);
    _starfield_colors.resize(2048);
    for (std::size_t ii = 0; ii * 2 < _starfield_points.size(); ++ii) {
        _starfield_points[ii * 2].x = r.uniform_real();
        _starfield_points[ii * 2].y = r.uniform_real();
        _starfield_points[ii * 2].z = 0.f;
        _starfield_colors[ii * 2] = color3(1,1,1) * r.uniform_real();
        // duplicate all points with z-offset to be used as star streaks
        _starfield_points[ii * 2 + 1] = _starfield_points[ii * 2 + 0];
        _starfield_points[ii * 2 + 1].z = 1.f;
        _starfield_colors[ii * 2 + 1] = _starfield_colors[ii * 2 + 0];
    }

    float k = 2.f * math::pi<float> / countof(_costbl);
    for (int ii = 0; ii < countof(_costbl); ++ii) {
        _sintbl[ii] = std::sin(k * ii);
        _costbl[ii] = std::cos(k * ii);
    }

    _timers.resize((_window->size().x >> 1));
    _timer_index = 0;

    return result::success;
}

//------------------------------------------------------------------------------
void system::shutdown()
{
    destroy_framebuffer();
    _fonts.clear();
}

//------------------------------------------------------------------------------
void system::begin_frame()
{
    _timers[_timer_index % _timers.size()].begin_frame = time_value::current();

    _framebuffer.draw();
    _framebuffer.draw_buffer(GL_COLOR_ATTACHMENT0);

    glClear(GL_COLOR_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void system::end_frame()
{
    config::scalar kernel_radius("r_bloomKernelRadius", .02f, config::archive, "");
    config::scalar bloom_alpha("r_bloomAlpha", .2f, 0, "");

    if (_graph) {
        draw_timers();
    }

    if (_bloom && _bloom_framebuffers.size()) {
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        _bloom_vao.bind();
        _bloom_downsample->program().use();

        if (_bloom_resolve.name()) {
            _bloom_resolve.blit(_framebuffer, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            _bloom_resolve.color_attachment().bind();
        } else {
            _framebuffer.color_attachment().bind();
        }

        _bloom_framebuffers[0].draw();

        glViewport(0, 0, _framebuffer.width() >> 1, _framebuffer.height() >> 1);
#if 1
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);
#else
        _bloom_framebuffers[0].blit(_framebuffer,
            0, 0, _framebuffer.width(), _framebuffer.height(),
            0, 0, _bloom_framebuffers[0].width(), _bloom_framebuffers[0].height(),
            GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif

        for (std::size_t ii = 1, sz = _bloom_framebuffers.size(); ii < sz; ++ii) {
            _bloom_framebuffers[ii].draw();
            _bloom_framebuffers[ii - 1].color_attachment().bind();

            glViewport(0, 0, _framebuffer.width() >> (ii + 1), _framebuffer.height() >> (ii + 1));
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);
        }
#if 1
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        _bloom_upsample->program().use();
        _bloom_upsample->program().uniform(0, kernel_radius);

        glBlendColor(0, 0, 0, bloom_alpha);
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

        for (std::size_t ii = 0, sz = _bloom_framebuffers.size(); ii < sz - 1; ++ii) {
            _bloom_framebuffers[sz - ii - 2].draw();
            _bloom_framebuffers[sz - ii - 1].color_attachment().bind();

            glViewport(0, 0, _framebuffer.width() >> (sz - ii - 1), _framebuffer.height() >> (sz - ii - 1));
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);

            // set kernel radius for next pass
            _bloom_upsample->program().uniform(0, kernel_radius / (2 << ii));
        }

        if (_bloom_resolve.name()) {
            _bloom_resolve.draw();
        } else {
            _framebuffer.draw();
        }
        _bloom_framebuffers[0].color_attachment().bind();

        glViewport(0, 0, _framebuffer.width(), _framebuffer.height());
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);
#else
        gl::framebuffer().draw_buffer(GL_BACK);
        gl::framebuffer().blit(_bloom_framebuffers[0],
            0, 0, _bloom_framebuffers[0].width(), _bloom_framebuffers[0].height(),
            0, 0, _window->size().x, _window->size().y,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif

        glViewport(0, 0, _framebuffer.width(), _framebuffer.height());

        gl::program().use();
        gl::vertex_array().bind();

        if (_bloom_resolve.name()) {
            gl::framebuffer().draw_buffer(GL_BACK);
            gl::framebuffer().blit(_bloom_resolve,
                0, 0, _bloom_resolve.width(), _bloom_resolve.height(),
                0, 0, _window->size().x, _window->size().y,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);
        } else {
            gl::framebuffer().draw_buffer(GL_BACK);
            gl::framebuffer().blit(_framebuffer,
                0, 0, _framebuffer.width(), _framebuffer.height(),
                0, 0, _window->size().x, _window->size().y,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }

    } else {
        gl::framebuffer().draw_buffer(GL_BACK);
        gl::framebuffer().blit(_framebuffer,
            0, 0, _framebuffer.width(), _framebuffer.height(),
            0, 0, _window->size().x, _window->size().y,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }


    _timers[_timer_index % _timers.size()].end_frame = time_value::current();

    _window->end_frame();

    _timers[_timer_index % _timers.size()].swap_buffer = time_value::current();
    ++_timer_index;

    if (_framebuffer_width.modified()
            || _framebuffer_height.modified()
            || _framebuffer_samples.modified()
            || _framebuffer_scale.modified()
            || _bloom.modified()) {
        resize(_window->size());
    }
}

//------------------------------------------------------------------------------
void system::set_view(render::view const& view)
{
    _view = view;
    _view_bounds = bounds::from_center(view.origin, view.size);
    set_default_state();
}

//------------------------------------------------------------------------------
void system::resize(vec2i size)
{
    if (!glBlendColor) {
        return;
    }

    vec2i actual_size;
    actual_size.x = _framebuffer_width ? _framebuffer_width : static_cast<int>(size.x * _framebuffer_scale);
    actual_size.y = _framebuffer_height ? _framebuffer_height : static_cast<int>(size.y * _framebuffer_scale);

    GLint max_samples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
    GLint num_samples = actual_size != size ? 0
        : _framebuffer_samples == -1 ? max_samples
        : std::min<int>(max_samples, _framebuffer_samples);

    create_framebuffer(actual_size, num_samples);

    _framebuffer_width.reset();
    _framebuffer_height.reset();
    _framebuffer_samples.reset();
    _framebuffer_scale.reset();
    _bloom.reset();

    create_default_font();
    set_default_state();

    // Intel drivers do not invalidate the default framebuffer dimensions until
    // it is cleared. Do it explicitly now, otherwise framebuffer operations
    // will not apply to the correct region of the default framebuffer.
    gl::framebuffer().draw();
    glClear(0);
}

//------------------------------------------------------------------------------
void system::create_default_font()
{
    int size = static_cast<int>((12.f / 480.f) * float(_framebuffer.height()));
    _font_scale = static_cast<float>(size) / static_cast<float>(_font_size);

    if (!_default_font || !_default_font->compare("Tahoma", _font_size)) {
        _default_font = std::make_unique<render::font>(this, "Tahoma", _font_size);
    }

    if (!_monospace_font || !_monospace_font->compare("Consolas", _font_size)) {
        _monospace_font = std::make_unique<render::font>(this, "Consolas", _font_size);
    }
}

//------------------------------------------------------------------------------
void system::set_default_state()
{
    glDisable(GL_TEXTURE_2D);

    glClearColor(0, 0, 0, 0.1f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

    glEnable(GL_POINT_SMOOTH );
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glPointSize(2.0f);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (!_view.viewport.empty()) {
        glViewport(
            _view.viewport.mins().x,
            _view.viewport.mins().y,
            _view.viewport.size().x,
            _view.viewport.size().y
        );
    } else {
        glViewport(0, 0, _framebuffer.width(), _framebuffer.height());
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    vec2 view_min = _view.origin - _view.size * 0.5f;
    vec2 view_max = _view.origin + _view.size * 0.5f;

    if (_view.raster) {
        glOrtho(view_min.x, view_max.x, view_max.y, view_min.y, -99999, 99999);
    } else {
        glOrtho(view_min.x, view_max.x, view_min.y, view_max.y, -99999, 99999);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(_view.origin.x, _view.origin.y, 0);
    glRotatef(math::rad2deg(_view.angle), 0, 0, -1);
    glTranslatef(-_view.origin.x, -_view.origin.y, 0);
}

//------------------------------------------------------------------------------
void system::create_framebuffer(vec2i size, int samples)
{
    GLenum format = _bloom ? GL_RGBA16F : GL_RGBA8;

    if (samples) {
        _framebuffer = gl::framebuffer(samples, size.x, size.y, {
            {gl::attachment_type::color, format},
            {gl::attachment_type::depth_stencil, GL_DEPTH24_STENCIL8},
        });
    } else {
        _framebuffer = gl::framebuffer(size.x, size.y, {
            {gl::attachment_type::color, format},
            {gl::attachment_type::depth_stencil, GL_DEPTH24_STENCIL8},
        });
    }

    if (_bloom) {
        _bloom_framebuffers.resize(8);
        for (std::size_t ii = 0, sz = _bloom_framebuffers.size(); ii < sz; ++ii) {
            _bloom_framebuffers[ii] = gl::framebuffer(size.x >> (ii + 1), size.y >> (ii + 1), {
                {gl::attachment_type::color, GL_RGBA16F}
            });
            _bloom_framebuffers[ii].color_attachment().parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _bloom_framebuffers[ii].color_attachment().parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        if (samples) {
            _bloom_resolve = gl::framebuffer(size.x, size.y, {
                {gl::attachment_type::color, format},
                {gl::attachment_type::depth_stencil, GL_DEPTH24_STENCIL8},
            });
        } else {
            _bloom_resolve = gl::framebuffer();
        }
    } else {
        _bloom_framebuffers.clear();
    }
}

//------------------------------------------------------------------------------
void system::destroy_framebuffer()
{
    _framebuffer = gl::framebuffer();
}

//------------------------------------------------------------------------------
void system::draw_timers() const
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float dx = time_delta::from_hertz(60).to_seconds() * _timers.size();
    float dy = time_delta::from_hertz(60).to_seconds();

    glTranslatef(1.f, -1.f, 0);
    glScalef(-2.f / dx, (1.f / 6.f) / dy, 1);

    // red - render time (begin_frame -> end_frame)
    glBegin(GL_LINE_STRIP);
        glColor4f(1,0,0,.8f);
        for (std::size_t ii = 0, sz = _timers.size(); ii < _timer_index && ii < sz; ++ii) {
            time_delta tx = _timers[(_timer_index - 1) % sz].end_frame
                            - _timers[(_timer_index - ii - 1) % sz].end_frame;
            time_delta ty = _timers[(_timer_index - ii - 1) % sz].end_frame
                            - _timers[(_timer_index - ii - 1) % sz].begin_frame;

            glVertex2f(tx.to_seconds(), ty.to_seconds());
        }
    glEnd();

    // green - game time (previous swap_buffer -> begin_frame)
    glBegin(GL_LINE_STRIP);
        glColor4f(0,1,0,.8f);
        for (std::size_t ii = 0, sz = _timers.size(); ii + 1 < _timer_index && ii + 1 < sz; ++ii) {
            time_delta tx = _timers[(_timer_index - 1) % sz].end_frame
                            - _timers[(_timer_index - ii - 1) % sz].end_frame;
            time_delta ty = _timers[(_timer_index - ii - 1) % sz].begin_frame
                            - _timers[(_timer_index - ii - 2) % sz].swap_buffer;

            glVertex2f(tx.to_seconds(), ty.to_seconds());
        }
    glEnd();

    // blue - frame time (previous swap_buffer -> swap_buffer)
    glBegin(GL_LINE_STRIP);
        glColor4f(0,.5f,1,.8f);
        for (std::size_t ii = 0, sz = _timers.size(); ii + 1 < _timer_index && ii + 1 < sz; ++ii) {
            time_delta tx = _timers[(_timer_index - 1) % sz].end_frame
                            - _timers[(_timer_index - ii - 1) % sz].end_frame;
            time_delta ty = _timers[(_timer_index - ii - 1) % sz].swap_buffer
                            - _timers[(_timer_index - ii - 2) % sz].swap_buffer;

            glVertex2f(tx.to_seconds(), ty.to_seconds());
        }
    glEnd();

    // white - 60 Hz marker
    glBegin(GL_LINES);
        glColor4f(1,1,1,.8f);
        glVertex2f(0.f, dy);
        glVertex2f(dx, dy);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

} // namespace render
