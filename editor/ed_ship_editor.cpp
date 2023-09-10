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
    , _stop{vec2_zero}
    , _curvature(0)
    , _start_section(SIZE_MAX)
    , _has_section(false)
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

    //_network.insert_edge(
    //    clothoid::segment::from_line(vec2(-24,-4), vec2(1,0), 8)
    //);
    //_network.insert_edge(
    //    clothoid::segment::from_line(vec2(16,4), vec2(1,0), 8)
    //);
    //_network.insert_edge(clothoid::segment::from_arc(vec2(-24, 12), vec2(1, 0), 8, .1f));
    //_network.insert_edge(clothoid::segment::from_arc(vec2(24, 12), vec2(1, 0), 8, -.1f));

    _network.insert_edge(clothoid::segment::from_transition(vec2(-20, 12), vec2(1, 0), 8, .0f, .2f));
    _network.insert_edge(clothoid::segment::from_transition(vec2(-8, 12), vec2(1, 0), 8, .1f, .2f));
    /**/_network.insert_edge(clothoid::segment::from_transition(vec2(4, 12), vec2(1, 0), 8, .2f, .1f));
    /**/_network.insert_edge(clothoid::segment::from_transition(vec2(16, 12), vec2(1, 0), 8, .2f, .0f));

    /**/_network.insert_edge(clothoid::segment::from_transition(vec2(-20, -12), vec2(1, 0), 8, .0f, -.2f));
    /**/_network.insert_edge(clothoid::segment::from_transition(vec2(-8, -12), vec2(1, 0), 8, -.1f, -.2f));
    _network.insert_edge(clothoid::segment::from_transition(vec2(4, -12), vec2(1, 0), 8, -.2f, -.1f));
    _network.insert_edge(clothoid::segment::from_transition(vec2(16, -12), vec2(1, 0), 8, -.2f, .0f));

    /**/_network.insert_edge(clothoid::segment::from_transition(vec2(-8, 0), vec2(1, 0), 8, .2f, -.2f));
    _network.insert_edge(clothoid::segment::from_transition(vec2(4, 0), vec2(1, 0), 8, -.2f, .2f));
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
        float s0 = n > 0 ? _cspline_length[n - 1] : 0.f;

        // a = v*v/r
        // v = sqrt(a*r)
        _max_speed[_graph_index] = min(kMaxSpeed, sqrt(kMaxLateral / abs(k)));
        _length[_graph_index] = s0 + cc.evaluate_arclength(t);
        float s = calculate_max_speed(_length[_graph_index]);
        if (_graph_index > 0) {
            // clamp acceleration
            if ((s - _speed[_graph_index - 1]) > kMaxAccel * dt) {
                s = _speed[_graph_index - 1] + kMaxAccel * dt;
            }
            if ((_speed[_graph_index - 1] - s) > kMaxDecel * dt) {
                s = _speed[_graph_index - 1] - kMaxDecel * dt;
            }
            _forward_acceleration[_graph_index] = (s - _speed[_graph_index - 1]) / dt;
        } else {
            _forward_acceleration[_graph_index] = 0.f;
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

    for (clothoid::network::edge_index edge : _network.edges()) {
        draw_section(renderer, _network.get_segment(edge));
    }

    for (clothoid::network::node_index node : _network.nodes()) {
        renderer->draw_box(vec2(.3f), _network.get_node(node).position, color4(1,1,1,.5f));
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
    } else if (_state == state::arc_start) {
#if 0
        vec2 stop = cursor_to_world();
        if (_curvature == 0.f) {
            section s{section_type::line};
            s.p0 = _start;
            s.t0 = stop - _start;
            s.L = s.t0.normalize_length();

            draw_section(renderer, s);
        } else {
            vec2 dir = (stop - _start);
            float len = dir.normalize_length();
            float theta = asin(.5f * len * abs(_curvature));
            if (len > 2.f / abs(_curvature)) {
                len = 2.f / abs(_curvature);
                stop = _start + dir * len;
                theta = .5f * math::pi<float>;
            }
            vec2 mid = .5f * (stop + _start);
            vec2 center = mid + dir.cross(1.f) * cos(theta) / _curvature;

            section s{section_type::arc};
            s.p0 = _start;
            s.t0 = (_start - center).normalize().cross(copysign(1.f, _curvature));
            s.L = 2.f * theta / abs(_curvature);
            s.k0 = _curvature;

            draw_section(renderer, s);
        }
#else
        clothoid::segment s{};
        calculate_section(s);

        draw_section(renderer, s);

        clothoid::segment o{};
        calculate_section_offset(s, o, 1.f);
        draw_section(renderer, o);

#endif
    }
#endif

#if 0
    {
        auto cs = connect_sections(_sections[0], _sections[1]);
        for (std::size_t ii = 0; ii < cs.size(); ++ii) {
            draw_section(renderer, cs[ii]);
        }
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
    if (_cspline_max_speed.size()) {
        draw_graph(renderer, _cspline_max_speed, color4(1, .5f, 0, 1));
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

            case K_MWHEELUP:
                _curvature += (1.f / 256.f);
                break;

            case K_MWHEELDOWN:
                _curvature -= (1.f / 256.f);
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
    //_has_section = false;
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
    mat3 tx = mat3(v.size.x / _cspline_length.back(), 0.f, 0.f,
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
void ship_editor::draw_graph(render::system* renderer, std::vector<vec2> const& data, color4 color) const
{
    render::view const& v = renderer->view();
    mat3 tx = mat3(v.size.x / _cspline_length.back(), 0.f, 0.f,
        0.f, v.size.y / 100.f, 0.f,
        v.origin.x - .5f * v.size.x, v.origin.y - .35f * v.size.y, 1.f);

    for (std::size_t ii = 1, sz = data.size(); ii < sz; ++ii) {
        renderer->draw_line(
            data[ii - 1] * tx,
            data[ii] * tx,
            color,
            color);

        renderer->draw_box(
            vec2(0.175f),
            data[ii - 1] * tx,
            color);
    }
}

//------------------------------------------------------------------------------
void ship_editor::draw_section(render::system* renderer, clothoid::segment const& s) const
{
    constexpr color4 guide_color(.3f, .3f, .3f, .9f);

    switch (s.type()) {
        case clothoid::segment_type::line: {
            vec2 p0 = s.initial_position();
            vec2 p1 = s.final_position();
            renderer->draw_line(p0, p1, color4(1, 1, 1, 1), color4(1, 1, 1, 1));
            renderer->draw_box(vec2(.25f), p0, color4(0, 1, 0, 1));
            renderer->draw_box(vec2(.25f), p1, color4(0, 1, 0, 1));
            break;
        }

        case clothoid::segment_type::arc: {
            vec2 p0 = s.initial_position();
            vec2 t0 = s.initial_tangent();
            vec2 n0 = t0.cross(-1);
            float k0 = s.initial_curvature();
            float r = 1.f / k0;
            vec2 p1 = s.final_position();
            vec2 c = p0 + r * t0.cross(-1);

            for (int ii = 0; ii < 24; ++ii) {
                vec2 x0 = s.evaluate(s.length() * float(ii) / 24.f);
                vec2 x1 = s.evaluate(s.length() * float(ii + 1) / 24.f);
                renderer->draw_line(x0, x1, color4(1, 1, 1, 1), color4(1, 1, 1, 1));
            }

            renderer->draw_arc(c, abs(r), 0.f, 0.f, 2.f * math::pi<float>, guide_color);
            renderer->draw_line(p0, c, guide_color, guide_color);
            renderer->draw_line(p1, c, guide_color, guide_color);

            renderer->draw_box(vec2(.25f), p0, color4(0, 1, 0, 1));
            renderer->draw_box(vec2(.25f), p1, color4(0, 1, 0, 1));
            renderer->draw_box(vec2(.25f), c, color4(1, 0, 0, 1));
            break;
        }

        case clothoid::segment_type::transition: {
            float k0 = s.initial_curvature();
            vec2 p0 = s.evaluate(0);
            vec2 t0 = s.evaluate_tangent(0);
            vec2 n0 = t0.cross(-copysign(1.f, k0));
            float L = s.length();
            if (k0) {
                renderer->draw_arc(p0 + n0 / abs(k0), 1.f / abs(k0), 0.f, 0.f, 2.f * math::pi<float>, guide_color);
            } else {
                renderer->draw_line(p0, p0 - t0 * 40.f, guide_color, guide_color);
            }
            vec2 t1, n1;
            for (int ii = 0; ii < 24; ++ii) {
                vec2 p1 = s.evaluate(L * float(ii + 1) / 24.f);
                renderer->draw_line(p0, p1, color4(1,1,1,1), color4(1,1,1,1));
                t1 = s.evaluate_tangent(L * float(ii + 1) / 24.f);
                n1 = t1.cross(-copysign(1.f, s.evaluate_curvature(L * float(ii + 1) / 24.f)));
                //renderer->draw_line(p1, p1 + n1, color4(.5f, 1, 0, 1), color4(.5f, 1, 0, 1));
                //renderer->draw_line(p1, p1 + t1, color4(0, .5f, 1, 1), color4(0, .5f, 1, 1));
                p0 = p1;
            }
            float k1 = s.final_curvature();
            if (k1) {
                renderer->draw_arc(p0 + n1 / abs(k1), 1.f / abs(k1), 0.f, 0.f, 2.f * math::pi<float>, guide_color);
            } else {
                renderer->draw_line(p0, p0 + t1 * 40.f, guide_color, guide_color);
            }
            renderer->draw_box(vec2(.25f), s.final_position(), color4(0, 1, 0, 1));
            renderer->draw_box(vec2(.25f), s.initial_position(), color4(0, 1, 0, 1));
            renderer->draw_string(va("%.0f", L), s.initial_position(), color4(1, 1, 1, 1));
            break;
        }
    }
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
        _state = state::arc_start;
        _curvature = 0.f;
        _start_section = SIZE_MAX;
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
        float s0 = _cspline_length.size() ? _cspline_length.back() : 0.f;
        _cspline_length.push_back(s0 + c.to_coeff().evaluate_arclength(1.f));
        {
            std::vector<vec2> max_speed;
            linearize_max_speed(c, max_speed);
            for (std::size_t ii = 0, sz = max_speed.size(); ii < sz; ++ii) {
                _cspline_max_speed.push_back(vec2(s0, 0) + max_speed[ii]);
            }
        }
        _start = _stop;
        _state = state::started;
        _beta1 = .5f;
        _beta2 = .5f;
    } else if (_state == state::beta) {
        _state = state::started;
    } else if (_state == state::arc_start) {
        clothoid::segment s;
        calculate_section(s);
        auto edge = _network.insert_edge(s);

        _state = state::arc_start;
        _start = s.final_position();
        _start_tangent = s.final_tangent();
        _start_curvature = s.final_curvature();
        _start_section = edge;
        _has_section = false;
    }
#endif
}

//------------------------------------------------------------------------------
void ship_editor::on_rclick()
{
    if (_state == state::beta) {
        _state = state::started;
    } else if (_state == state::beta) {
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

//------------------------------------------------------------------------------
void ship_editor::calculate_section(clothoid::segment& s) const
{
    if (_start_section != clothoid::network::invalid_edge) {
#if 0
        if (!_curvature && !_start_curvature) {
            vec2 p1 = _start + _start_tangent * dot(_start_tangent, cursor_to_world() - _start);
            s.type = section_type::line;
            s.p0 = _start;
            s.t0 = _start_tangent;
            s.L = (p1 - _start).length();
        } else if (_curvature == _start_curvature) {
            s.type = section_type::arc;
            s.p0 = _start;
            s.t0 = _start_tangent;
            s.L = (cursor_to_world() - _start).length();
            s.k0 = _start_curvature;
        } else {
            s.type = section_type::clothoid;
            s.p0 = _start;
            s.t0 = _start_tangent;
            s.L = (cursor_to_world() - _start).length();
            s.k0 = _start_curvature;
            s.k1 = _curvature;
    }
#else
        if (!_has_section) {
            s = clothoid::segment::from_arc(
                _start,
                _start_tangent,
                (cursor_to_world() - _start).length(),
                _start_curvature);
            _section = s;
            _has_section = true;
        } else {
            vec2 end = _section.final_position();
            vec2 target = cursor_to_world();
            vec2 dir = target - _start;
            float L = _section.length() - dot(dir, (end - target)) / dir.length();
            float k1 = _section.final_curvature() - .1f * cross(dir, (end - target)) / dir.length_sqr();
            _section = clothoid::segment::from_transition(
                _start,
                _start_tangent,
                L,
                _start_curvature,
                k1);
#if 0
            {
                float kmin, kmax;
                float A = .5f * _section.L / (math::pi<float> / 2.f);
                float B = -1.f;
                float C = _section.k0;
                float det = B * B - 4.f * A * C;
                if (det > 0.f) {
                    float Q = -.5f * (B + copysign(sqrt(det), B));
                    kmin = min(Q / A, C / Q);
                    kmax = max(Q / A, C / Q);
                    if (_section.k1 < kmin) {
                        _section.k1 = kmin;
                    }
                    if (_section.k1 > kmax) {
                        _section.k1 = kmax;
                    }
                }
            }
#endif
            if (isnan(_section.length()) || isnan(_section.final_curvature())) {
                _has_section = false;
            }
            s = _section;
        }
        if (!_start_curvature) {
            vec2 p2 = cursor_to_world();
            vec2 p1 = _start + _start_tangent * dot(_start_tangent, p2 - _start);
            if ((p2 - p1).length_sqr() < 1.f) {
                s = clothoid::segment::from_line(
                    _start,
                    _start_tangent,
                    (p1 - _start).length());
                _section = s;
                _has_section = true;
            }
        } else {
            vec2 center = _start + _start_tangent.cross(-1.f) / _start_curvature;
            vec2 p2 = cursor_to_world();
            vec2 p1 = (p2 - center).normalize() / abs(_start_curvature) + center;
            if ((p2 - p1).length_sqr() < 1.f) {
                s = clothoid::segment::from_arc(
                    _start,
                    _start_tangent,
                    atan2f(cross(_start - center, p1 - center), dot(_start - center, p1 - center)) / _start_curvature,
                    _start_curvature);
                _section = s;
                _has_section = true;
            }
        }
#endif
    } else if (!_curvature) {
        vec2 t0 = cursor_to_world() - _start;
        float L = t0.normalize_length();
        s = clothoid::segment::from_line(
            _start, t0, L);
    } else {
        vec2 stop = cursor_to_world();
        vec2 dir = (stop - _start);
        float len = dir.normalize_length();
        float theta = asin(.5f * len * abs(_curvature));
        if (len > 2.f / abs(_curvature)) {
            len = 2.f / abs(_curvature);
            stop = _start + dir * len;
            theta = .5f * math::pi<float>;
        }
        vec2 mid = .5f * (stop + _start);
        vec2 center = mid + dir.cross(-1) * cos(theta) / _curvature;

        s = clothoid::segment::from_arc(
            _start,
            (_start - center).normalize().cross(-copysign(1.f, _curvature)),
            2.f * theta / abs(_curvature),
            _curvature);
    }
}

//------------------------------------------------------------------------------
void ship_editor::calculate_section_offset(clothoid::segment const& s, clothoid::segment& o, float offset) const
{
    switch (s.type()) {
        case clothoid::segment_type::line: {
            vec2 n0 = s.initial_tangent().cross(1);
            o = clothoid::segment::from_line(
                s.initial_position() + n0 * offset,
                s.initial_tangent(),
                s.length());
            break;
        }

        case clothoid::segment_type::arc: {
            vec2 n0 = s.initial_tangent().cross(1);
            float r0 = 1.f / s.initial_curvature();
            o = clothoid::segment::from_arc(
                s.initial_position() + n0 * offset,
                s.initial_tangent(),
                s.length() * (r0 + offset) / r0,
                1.f / (r0 + offset));
            break;
        }

        case clothoid::segment_type::transition: {
            vec2 n0 = s.initial_tangent().cross(1);
            vec2 n1 = s.final_tangent().cross(1);
            vec2 p1 = s.final_position() + n1 * offset;
            float k0, k1;
            if (s.initial_curvature()) {
                float r0 = 1.f / s.initial_curvature();
                k0 = 1.f / (r0 + offset);
            } else {
                k0 = 0;
            }
            if (s.final_curvature()) {
                float r1 = 1.f / s.final_curvature();
                k1 = 1.f / (r1 + offset);
            } else {
                k1 = 0;
            }
            o = clothoid::segment::from_transition(
                s.initial_position() + n0 * offset,
                s.initial_tangent(),
                s.length(), // fixme
                k0,
                k1);
            break;
        }
    }
}

//------------------------------------------------------------------------------
std::vector<clothoid::segment> ship_editor::connect_sections(clothoid::segment const& /*s1*/, clothoid::segment const& /*s2*/) const
{
#if 0
    vec2 p0 = s1.evaluate(s1.L);
    vec2 p1 = s2.evaluate(0);
    vec2 t0 = s1.evaluate_tangent(s1.L);
    vec2 t1 = s2.evaluate_tangent(0);

    // connect parallel tracks
    if (dot(t0, t1) > 0.999f) {
        vec2 p2 = .5f * (p0 + p1);

        clothoid::segment o1;
        o1.type = section_type::clothoid;
        o1.p0 = p0;
        o1.t0 = t0;
        o1.k0 = 0;
        o1.L = .25f * (p1 - p0).length();
        o1.k1 = 0;

        clothoid::segment o2;
        o2.type = section_type::clothoid;
        o2.k1 = 0;

        vec2 dir = (p2 - p0);

        for (int ii = 0; ii < 32; ++ii) {
            o2.p0 = o1.evaluate(o1.L);
            o2.t0 = o1.evaluate_tangent(o1.L);
            o2.L = o1.L;
            o2.k0 = o1.k1;

            vec2 mid = o2.evaluate(o2.L);

            o1.L -= dot(dir, (mid - p2)) / dir.length();
            o1.k1 -= .1f * cross(dir, (mid - p2)) / dir.length_sqr();
        }

        std::vector<clothoid::segment> sections;
        sections.push_back(o1);
        sections.push_back(o2);
        sections.push_back(o1);
        sections.push_back(o2);
        sections[2].p0 = sections[1].evaluate(sections[1].L);
        sections[2].t0 = sections[1].evaluate_tangent(sections[1].L);
        sections[2].k1 = -sections[1].k0;
        sections[3].p0 = sections[2].evaluate(sections[2].L);
        sections[3].t0 = sections[2].evaluate_tangent(sections[2].L);
        sections[3].k0 = sections[2].k1;
        return sections;
    }
#endif
    return {};
}

//------------------------------------------------------------------------------
void ship_editor::linearize_max_speed(ccurve const& c, std::vector<vec2>& max_speed) const
{
    ccurve_coeff cc = c.to_coeff();
    float s1 = cc.evaluate_arclength(1.f);
    float k0 = cc.evaluate_curvature(0.f);
    float k1 = cc.evaluate_curvature(1.f);

    max_speed.push_back(vec2(0.f, min(kMaxSpeed, sqrt(kMaxLateral / abs(k0)))));
    linearize_max_speed_r(cc, max_speed, 0.f, 1.f, 0.f, s1, k0, k1);
    max_speed.push_back(vec2(s1, min(kMaxSpeed, sqrt(kMaxLateral / abs(k1)))));
}

//------------------------------------------------------------------------------
void ship_editor::linearize_max_speed_r(ccurve_coeff const& cc, std::vector<vec2>& max_speed, float t0, float t1, float s0, float s1, float k0, float k1) const
{
    float t2 = .5f * (t0 + t1);
    float s2 = cc.evaluate_arclength(t2);
    float k2 = cc.evaluate_curvature(t2);

    vec2 a = vec2(s0, min(kMaxSpeed, sqrt(kMaxLateral / abs(k0))));
    vec2 b = vec2(s1, min(kMaxSpeed, sqrt(kMaxLateral / abs(k1))));
    vec2 c = vec2(s2, min(kMaxSpeed, sqrt(kMaxLateral / abs(k2))));

    float d = abs(cross(c - a, b - a)) / length(b - a);

    if (d > 1.f) {
        linearize_max_speed_r(cc, max_speed, t0, t2, s0, s2, k0, k2);
        max_speed.push_back(c);
        linearize_max_speed_r(cc, max_speed, t2, t1, s2, s1, k2, k1);
    } else {
        max_speed.push_back(c);
    }
}

//------------------------------------------------------------------------------
float ship_editor::calculate_max_speed(float s) const
{
    float smax = s + .5f * kMaxSpeed * kMaxSpeed / kMaxDecel;
    float vmax = kMaxSpeed;
    for (std::size_t ii = 0, sz = _cspline_max_speed.size(); ii < sz; ++ii) {
        if (_cspline_max_speed[ii][0] < s) {
            continue;
        } else if (_cspline_max_speed[ii][0] > smax) {
            break;
        }

        float ds = _cspline_max_speed[ii][0] - s;
        float v = _cspline_max_speed[ii][1];
        float v0 = sqrt(v * v + 2.f * ds * kMaxDecel);
        vmax = min(v0, vmax);
    }

    return vmax;
}

} // namespace editor
