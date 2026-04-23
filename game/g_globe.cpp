// game/g_globe.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_globe.h"
#include "cm_keys.h"

#include "render/gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
globe::globe()
    : _longitude(0)
    , _latitude(0)
    , _zoom(1e-2f)
    , _is_dragging(false)
    , _cursor(vec2_zero)
{
    _gshhg.load(gshhg::resolution::full);
}

//------------------------------------------------------------------------------
void globe::draw(render::system* renderer, time_value /*time*/)
{
    render::view view{};
    view.size = vec2(640, 360) * _zoom;

    if (!_vbo.name()) {
        _vbo = render::gl::vertex_buffer<vec3>(
            render::gl::buffer_usage::static_,
            render::gl::buffer_access::draw,
            _gshhg.vertices().size(),
            _gshhg.vertices().data());
        _vao = render::gl::vertex_array({
            render::gl::vertex_array_attrib{3, GL_FLOAT, render::gl::vertex_attrib_type::float_, 0}});
        _vao.bind_buffer(_vbo, 0);
    }

    renderer->set_view(view);
    renderer->draw_arc(vec2_zero, 1.f, 0, 0, math::twopi, color4(1,1,1,.5f));

    float cp = cos(_latitude);
    float sp = sin(_latitude);
    vec3 v = vec3(cos(_longitude) * cp, sin(_longitude) * cp, sp);
    vec3 r = vec3(-sin(_longitude), cos(_longitude), 0);
    vec3 u = cross(v, r);

    GLfloat mm[16] = {
        r.x, u.x, v.x, 0,
        r.y, u.y, v.y, 0,
        r.z, u.z, v.z, 0,
        0, 0, 0, 1
    };

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(mm);

    glClearDepth(.5);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    _vao.bind();
    glColor4f(1,1,1,.5f);
    for (std::size_t ii = 0; ii < _gshhg.polygons().size(); ++ii) {
        // Skip everything except islands/continents (1) and Antarctic ice-front (5)
        if ((_gshhg.polygons()[ii].flags & 255) != 1 && (_gshhg.polygons()[ii].flags & 255) != 5) {
            continue;
        }
        glDrawArrays(
            GL_LINE_LOOP,
            _gshhg.polygons()[ii].start,
            _gshhg.polygons()[ii].count);
    }
    glDisable(GL_DEPTH_TEST);
    glPopMatrix();
    render::gl::vertex_array().bind();
}

//------------------------------------------------------------------------------
bool globe::key_event(int key, bool down)
{
    if (key == K_MOUSE2) {
        _is_dragging = down;
        return true;
    } else if (down && key == K_MWHEELDOWN) {
        _zoom *= 1.2f;
    } else if (down && key == K_MWHEELUP) {
        _zoom *= (1.f / 1.2f);
    }
    return false;
}

//------------------------------------------------------------------------------
void globe::cursor_event(vec2 position)
{
    if (_is_dragging) {
        vec2 delta = position - _cursor;
        _longitude -= delta.x * _zoom;
        _latitude = clamp(_latitude + delta.y * _zoom, -.5f * math::pi, .5f * math::pi);
    }

    _cursor = position;
}

} // namespace game
