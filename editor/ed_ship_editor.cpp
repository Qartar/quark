// ed_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "ed_ship_editor.h"
#include "r_image.h"
#include "r_window.h"

#include <algorithm>
#include <iterator>

////////////////////////////////////////////////////////////////////////////////
namespace editor {

namespace {

std::vector<std::size_t> sorted_union(std::vector<std::size_t> const& a, std::vector<std::size_t> const& b)
{
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(out));

    return out;
}

std::vector<std::size_t> sorted_difference(std::vector<std::size_t> const& a, std::vector<std::size_t> const& b)
{
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());
    std::set_difference(a.begin(), a.end(),
                        b.begin(), b.end(),
                        std::back_inserter(out));

    return out;
}

} // anonymous namespace

//------------------------------------------------------------------------------
ship_editor::ship_editor()
    : _view{}
    , _cursor{}
    , _anim{}
    , _last_time{}
    , _graph_index(0)
    , _state{state::ready}
    , _start{vec2_zero}
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

}

//------------------------------------------------------------------------------
ship_editor::~ship_editor()
{
}

//------------------------------------------------------------------------------
vec2 ship_editor::cursor_to_world() const
{
    //return _view.origin + _view.size * (_cursor / 
    //    vec2(_view.viewport.maxs() - _view.viewport.mins()) - vec2(.5f,.5f));
    return _view.origin + _view.size * _cursor;
}

//------------------------------------------------------------------------------
void ship_editor::draw(render::system* renderer, time_value time) const
{
    renderer->set_view(_view);

    float dt = (time - _last_time).to_seconds();
    _last_time = time;

    if (_cspline.size()) {
        std::size_t n = (std::size_t)_anim;
        while (n >= _cspline.size()) {
            n -= _cspline.size();
            _anim -= _cspline.size();
            _graph_index = 0;
        }
        ccurve_coeff cc = _cspline[n].to_coeff();
        float t = _anim - n;

        draw_box(renderer, _cspline[n], t, vec2(0.25f), color4(1,1,1,1));

        if (_graph_index >= _forward_acceleration.size()) {
            _forward_acceleration.push_back({});
            _lateral_acceleration.push_back({});
            _speed.push_back({});
            _max_speed.push_back({});
            _length.push_back({});
        }

        float k = cc.evaluate_curvature(t);

        // a = v*v/r
        // v = sqrt(a*r)
        _max_speed[_graph_index] = min(10.f, sqrt(10.f / abs(k)));
        float s = _max_speed[_graph_index];
        if (_graph_index > 0) {
            if ((s - _speed[_graph_index - 1]) / dt > 1.f) {
                s = _speed[_graph_index - 1] + 1.f * dt;
            }
            _forward_acceleration[_graph_index] = (s - _speed[_graph_index - 1]) / dt;
            _length[_graph_index] = s * dt + _length[_graph_index - 1];
        } else {
            s = 0.f;
            _forward_acceleration[_graph_index] = 0.f;
            _length[_graph_index] = s * dt;
        }
        _lateral_acceleration[_graph_index] = square(s) * k;
        _speed[_graph_index] = s;
        ++_graph_index;

        _anim += s * dt / cc.evaluate_tangent(t).length();
    }

#if 0
    for (std::size_t ii = 0, sz = _control_points.size(); ii < sz; ii += 3) {
        if (ii + 3 > sz) {
            vec2 b, c = cursor_to_world();
            calculate_qspline_points(b, c);
            draw_qspline(renderer, _control_points[ii], b, c);
        } else {
            draw_qspline(renderer,
                _control_points[ii],
                _control_points[ii + 1],
                _control_points[ii + 2]);
        }
    }
#else
    for (ccurve const& c : _cspline) {
        draw_cspline(renderer, c);
    }

    if (_state == state::started || _state == state::beta) {
        ccurve c = {
            _start,
            vec2_zero,
            vec2_zero,
            (_state == state::started)
                ? cursor_to_world()
                : _stop
        };
        calculate_cspline_points(c);
        draw_cspline(renderer, c);
    }
#endif

    //
    //  draw graphs
    //

    if (_graph_index) {
        draw_graph(renderer, _forward_acceleration, color4(1, 1, 1, 1));
        draw_graph(renderer, _lateral_acceleration, color4(0, 1, 1, 1));
        draw_graph(renderer, _speed, color4(1, 1, 0, 1));
        draw_graph(renderer, _max_speed, color4(1, 0, 0, 1));
    }
}

//------------------------------------------------------------------------------
bool ship_editor::key_event(int key, bool down)
{
    if (!down) {
        switch (key) {
            case K_MOUSE1:
                on_click();
                break;

            case K_MOUSE2:
                on_rclick();
                break;

            case K_CTRL:
                break;

            case K_SHIFT:
                break;

            default:
                break;
        }

        return false;
    }

    switch (key) {
        case K_MOUSE1:
            break;

        case K_MOUSE2:
            break;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::cursor_event(vec2 position)
{
    _cursor = position;
    if (_state == state::beta) {
        vec2 p = cursor_to_world();
        _beta1 = .5f + (p.x - _stop.x) * .1f;
        _beta2 = .5f + (p.y - _stop.y) * .1f;
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_qspline(render::system* renderer, qcurve const& q) const
{
    // draw control points
    renderer->draw_box(vec2(0.5f), q.a, color4(1,1,0,1));
    renderer->draw_box(vec2(0.5f), q.b, color4(0,1,0,1));
    renderer->draw_box(vec2(0.5f), q.c, color4(1,1,0,1));

    vec2 p0 = q.a;
    for (int ii = 0; ii < 16; ++ii) {
        float t = float(ii) / 15.f;
        float s = 1.f - t;
        vec2 m01 = q.a * s + q.b * t;
        vec2 m12 = q.b * s + q.c * t;
        vec2 p = m01 * s + m12 * t;
        renderer->draw_line(p0, p, color4(1,1,1,1), color4(1,1,1,1));
        p0 = p;
    }
}

//------------------------------------------------------------------------------
color4 curvature_color(float k)
{
    const float s = 10.f;
    float H = 6.f * (s * k - floor(s * k));
    float r = H - floor(H);

    float P = 0.f;
    float Q = 1.f - r;
    float T = r;

    if (H < 1.f) {
        return color4(1.f, T, P, 1.f);
    } else if (H < 2.f) {
        return color4(Q, 1.f, P, 1.f);
    } else if (H < 3.f) {
        return color4(P, 1.f, T, 1.f);
    } else if (H < 4.f) {
        return color4(P, Q, 1.f, 1.f);
    } else if (H < 5.f) {
        return color4(T, P, 1.f, 1.f);
    } else {
        return color4(1.f, P, Q, 1.f);
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_cspline(render::system* renderer, ccurve const& c) const
{
    // draw control points
    renderer->draw_box(vec2(0.25f), c.a, color4(1,1,0,1));
    renderer->draw_box(vec2(0.25f), c.b, color4(0,1,0,1));
    renderer->draw_box(vec2(0.25f), c.c, color4(0,1,0,1));
    renderer->draw_box(vec2(0.25f), c.d, color4(1,1,0,1));

    ccurve_coeff cc = c.to_coeff();

    vec2 p0 = c.a;
    vec2 n0 = (c.b - c.a).normalize().cross(1);
    float k0 = cc.evaluate_curvature(0.f) * 5.f;

    renderer->draw_line(p0, p0 + n0 * k0,
        color4(1,1,1,.25f),
        color4(1,1,1,.25f));

    for (int ii = 0; ii < 128; ++ii) {
        float t = float(ii) / 127.f;
        vec2 p = cc.evaluate(t);
        vec2 n = cc.evaluate_tangent(t).normalize().cross(1);
        float k = cc.evaluate_curvature(t) * 5.f;

        renderer->draw_line(p0, p,
            color4(1,1,1,.75f),
            color4(1,1,1,.75f));
        renderer->draw_line(p0 + n0 * k0, p + n * k,
            color4(1,1,1,.5f),
            color4(1,1,1,.5f));

        if (ii % 4 == 3) {
            renderer->draw_line(p, p + n * k,
                color4(1,1,1,.25f),
                color4(1,1,1,.25f));
        }

        p0 = p;
        k0 = k;
        n0 = n;
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_box(render::system* renderer, ccurve const& c, float t, vec2 size, color4 color) const
{
    ccurve_coeff cc = c.to_coeff();

    vec2 v0 = cc.evaluate(t);
    vec2 vx = .5f * cc.evaluate_tangent(t).normalize();
    vec2 vy = vx.cross(1.f);

    vec2 points[4] {
        v0 - vx * size.x - vy * size.y,
        v0 + vx * size.x - vy * size.y,
        v0 - vx * size.x + vy * size.y,
        v0 + vx * size.x + vy * size.y,
    };

    color4 const colors[4] = { color, color, color, color };
    int const indices[6] = { 0, 1, 2, 1, 3, 2 };

    renderer->draw_triangles(points, colors, indices, 6);
}

//------------------------------------------------------------------------------
void ship_editor::draw_graph(render::system* renderer, std::vector<float> const& data, color4 color) const
{
    render::view const& v = renderer->view();
    mat3 tx = mat3(v.size.x / _length.back(), 0.f, 0.f,
                   0.f, v.size.y / 100.f, 0.f,
                   v.origin.x - .5f * v.size.x, v.origin.y - .35f * v.size.y, 1.f);

    for (std::size_t ii = 1, sz = data.size(); ii < sz; ++ii) {
        renderer->draw_line(
            vec2(_length[ii - 1], data[ii - 1]) * tx,
            vec2(_length[ii], data[ii]) * tx,
            color,
            color);
    }

    renderer->draw_box(
        vec2(0.25f),
        vec2(_length[_graph_index - 1], data[_graph_index - 1]) * tx,
        color);
}

//------------------------------------------------------------------------------
void ship_editor::on_click()
{
#if 0
    if (_control_points.size() % 3 == 0) {
        // start a new qspline at the cursor
        _control_points.push_back(cursor_to_world());
    } else if (_control_points.size() % 3 == 1) {
        // finish a new qspline at the cursor
        vec2 b, c = cursor_to_world();
        calculate_qspline_points(b, c);
        _control_points.push_back(b);
        _control_points.push_back(c);
        // start a new qspline at the cursor
        _control_points.push_back(c);
    }
#else
    if (_state == state::ready) {
        _start = cursor_to_world();
        _state = state::started;
    } else if (_state == state::started) {
        _stop = cursor_to_world();
        _state = state::beta;
    } else if (_state == state::beta) {
        // finish a new cspline at the cursor
        ccurve c = {
            _start,
            vec2_zero,
            vec2_zero,
            _stop
        };
        calculate_cspline_points(c);
        _cspline.push_back(c);
        _start = _stop;
        _state = state::started;
        _beta1 = .5f;
        _beta2 = .5f;
    }
#endif
}

//------------------------------------------------------------------------------
void ship_editor::on_rclick()
{
    if (_state == state::beta) {
        _state = state::started;
    }
}

//------------------------------------------------------------------------------
void ship_editor::calculate_qspline_points(qcurve& q) const
{
#if 0
    std::size_t sz = _control_points.size();
    for (std::size_t ii = sz - sz % 3; ii > 0; ii -= 3) {
        if (_control_points[ii - 1] == _control_points[sz - 1]) {
            // b = p[n-1] + (p[n-1] - p[n-2])
            q.b = _control_points[ii - 1] * 2.f - _control_points[ii - 2];
            return;
        }
    }

    q.b = .5f * (_control_points[sz - 1] + q.c);
#else
    (void)q;
#endif
}

//------------------------------------------------------------------------------
void ship_editor::calculate_cspline_points(ccurve& c) const
{
    // G1 : P4 = P3 + (P3 - P2) * b1
    // G2 : P5 = P3 + (P3 - P2) * (2b1 + b1^2 + b2/2) + (P1 - P2)b1^2

    const float beta1 = _beta1;
    const float beta2 = _beta2;

    const float c1 = 2.f * beta1 + beta1 * beta1 + .5f * beta2;
    const float c2 = beta1 * beta1;

    for (ccurve const& d : _cspline) {
        if (d.d == c.a) {
#if 0
            // b = p[n-1] + (p[n-1] - p[n-2])
            c.b = d.d * 2.f - d.c;
            vec2 r = c.d - d.d;
            vec2 v = c.b - d.d;
            vec2 x = r.dot(v) * r / r.length_sqr();
            vec2 y = v - x;

            float s = sqrt((c.d - d.d).length_sqr()
                / (d.d - d.a).length_sqr());

            c.c = c.d - x * s + y * s;
#else
            c.b = d.d + (d.d - d.c) * beta1;
            c.c = d.d + (d.d - d.c) * c1 + (d.b - d.c) * c2;
#endif
            return;
        }
    }

    c.b = (2.f / 3.f) * c.a + (1.f / 3.f) * c.d;
    c.c = (1.f / 3.f) * c.a + (2.f / 3.f) * c.d;
}

} // namespace editor
