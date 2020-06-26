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

#include "r_light.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
system::system(render::window* window)
    : _framebuffer_width("r_width", 0, config::archive, "framebuffer width, or 0 to use window width")
    , _framebuffer_height("r_height", 0, config::archive, "framebuffer height, or 0 to use window height")
    , _framebuffer_scale("r_scale", 1, config::archive, "framebuffer scale if using window dimensions")
    , _framebuffer_samples("r_samples", -1, config::archive, "framebuffer samples, or -1 to use maximum supported")
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

    _light = std::make_unique<light>();

    string::buffer info_log;
    auto vsh = gl::shader(gl::shader_stage::vertex,
R"(
#version 110
varying vec4 color;
void main() {
    color = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)"
    );

    if (vsh.compile_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }

    auto fsh = gl::shader(gl::shader_stage::fragment,
R"(
#version 110
varying vec4 color;
void main() {
    gl_FragColor = color;
}
)"
    );

    if (fsh.compile_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }

    _program = gl::program(vsh, fsh);

    if (_program.link_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }

    if (_program.validate_status(info_log)) {
        if (info_log.length()) {
            log::warning("%.*s", info_log.length(), info_log.begin());
        }
    } else if (info_log.length()) {
        log::error("%.*s", info_log.length(), info_log.begin());
    }

    _program.use();

    _view.size = vec2(_window->size());
    _view.origin = _view.size * 0.5f;
    _view.viewport = {};

    resize(_window->size());
    _light->resize(_window->size());

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
    _light->render(_framebuffer);

    if (_graph) {
        draw_timers();
    }

    gl::framebuffer().draw_buffer(GL_BACK);
    gl::framebuffer().blit(_framebuffer,
        0, 0, _framebuffer.width(), _framebuffer.height(),
        0, 0, _window->size().x, _window->size().y,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);

    _timers[_timer_index % _timers.size()].end_frame = time_value::current();

    _window->end_frame();

    _timers[_timer_index % _timers.size()].swap_buffer = time_value::current();
    ++_timer_index;

    if (_framebuffer_width.modified()
            || _framebuffer_height.modified()
            || _framebuffer_samples.modified()
            || _framebuffer_scale.modified()) {
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

    if (!_default_font || !_default_font->compare("Tahoma", size)) {
        _default_font = std::make_unique<render::font>("Tahoma", size);
    }

    if (!_monospace_font || !_monospace_font->compare("Consolas", size)) {
        _monospace_font = std::make_unique<render::font>("Consolas", size);
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
    if (samples) {
        _framebuffer = gl::framebuffer(samples, size.x, size.y, {
            {gl::attachment_type::color, GL_RGBA8},
            {gl::attachment_type::depth_stencil, GL_DEPTH24_STENCIL8},
        });
    } else {
        _framebuffer = gl::framebuffer(size.x, size.y, {
            {gl::attachment_type::color, GL_RGBA8},
            {gl::attachment_type::depth_stencil, GL_DEPTH24_STENCIL8},
        });
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
