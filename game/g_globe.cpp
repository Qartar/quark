// game/g_globe.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_globe.h"
#include "cm_keys.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
globe::globe()
    : _longitude(0)
    , _latitude(0)
    , _zoom(1e-2f)
    , _is_dirty(false)
    , _is_dragging(false)
    , _cursor(vec2_zero)
{
    _vertices.resize(X * Y);
    _colors.resize(X * Y);
    _indices.resize((X - 1) * (Y - 1) * 6);

    for (int jj = 0; jj < Y; ++jj) {
        for (int ii = 0; ii < X; ++ii) {
            _vertices[jj * X + ii] = vec2((ii - X / 2) * (640.f / X), (Y / 2 - jj) * (360.f / Y));
            _colors[jj * X + ii] = color4(1,1,1,1);
        }
    }

    int* idx = _indices.data();
    for (int jj = 0; jj + 1 < Y; ++jj) {
        for (int ii = 0; ii + 1 < X; ++ii) {
            *idx++ = (jj + 0) * X + (ii + 0);
            *idx++ = (jj + 1) * X + (ii + 0);
            *idx++ = (jj + 0) * X + (ii + 1);
            *idx++ = (jj + 1) * X + (ii + 0);
            *idx++ = (jj + 0) * X + (ii + 1);
            *idx++ = (jj + 1) * X + (ii + 1);
        }
    }

    resample();
}

//------------------------------------------------------------------------------
void globe::draw(render::system* renderer, time_value /*time*/)
{
    render::view view{};
    view.size = vec2(640, 360);

    if (_is_dirty) {
        resample();
    }

    renderer->set_view(view);
    renderer->draw_triangles(_vertices.data(), _colors.data(), _indices.data(), _indices.size());
}

//------------------------------------------------------------------------------
bool globe::key_event(int key, bool down)
{
    if (key == K_MOUSE2) {
        _is_dragging = down;
        return true;
    } else if (down && key == K_MWHEELDOWN) {
        _zoom *= 1.2f;
        _is_dirty = true;
    } else if (down && key == K_MWHEELUP) {
        _zoom *= (1.f / 1.2f);
        _is_dirty = true;
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
        _is_dirty = true;
    }

    _cursor = position;
}

//------------------------------------------------------------------------------
bool intersect_unit_sphere(vec3 p, vec3 v, vec3& i)
{
    float a = dot(v, v);
    float b = 2.f * dot(p, v);
    float c = dot(p, p) - 1.f;
    float d = b * b - 4.f * a * c;

    if (d < 0.f) {
        return false;
    } else if (d == 0.f) {
        i = p + v * -.5f * b / a;
        return true;
    } else {
        float q = -.5f * (b + std::copysign(std::sqrt(d), b));
        i = p + v * std::min(q / a, c / q);
        return true;
    }
}

//------------------------------------------------------------------------------
void globe::resample()
{
    float cp = cos(_latitude);
    float sp = sin(_latitude);
    vec3 v = vec3(cos(_longitude) * cp, sin(_longitude) * cp, sp);
    vec3 r = vec3(-sin(_longitude), cos(_longitude), 0) * _zoom;
    vec3 u = cross(v, r);

    for (int jj = 0; jj < Y; ++jj) {
        float dy = float(jj - Y / 2) * (320.f / float(Y));
        for (int ii = 0; ii < X; ++ii) {
            float dx = float(ii - X / 2) * (320.f / float(Y));

            vec3 p = v + r * dx - u * dy, i;
            if (!intersect_unit_sphere(p, -v, i)) {
                _colors[jj * X + ii] = color4(0,0,0,1);
            } else {
                float h = _topo.height(i);
                if (h > 0.f) {
                    _colors[jj * X + ii] = color4(.1f,.2f,.1f,1);
                } else {
                    _colors[jj * X + ii] = color4(.05f,.1f,.2f,1);
                }
            }
        }
    }

    _is_dirty = false;
}

} // namespace game
