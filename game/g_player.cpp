// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
std::vector<clothoid::segment> connect_linear_segments(vec2 p0, vec2 t0, vec2 p1, vec2 t1)
{
    float theta = acos(dot(t0, t1));
    mat2 m(t0.x, t0.y, -t0.y, t0.x);

    // normalized clothoid coordinates for the required angle
    vec2 s;
    float t = sqrt(theta / math::pi<float>);
    clothoid::segment::fresnel_integral(t, s.x, s.y);
    s *= math::pi<float>;
    if (cross(t0, t1) < 0) {
        s.y *= -1.f;
        theta *= -1.f;
    }
    // transformed coordinates of the combined normalized curves
    vec2 r = (s + vec2(s.x, -s.y) * mat2::rotate(theta)) * m;

    // calculate the maximum size of the transition curve
    float scale = min(cross(p1 - p0, t1) / cross(r, t1),
                      cross(p1 - p0, t0) / cross(r, t0));
    // length and curvature of the curves based on scaling parameter
    float length = scale * sqrt(abs(theta) * math::pi<float>);
    float curvature = theta / length;

    if (isnan(length) || length < 1.f) {
        return {};
    }

    // midpoint of the transition curves
    vec2 mid = p0 + s * m * scale;

    float den = cross(t0, t1);
    float u = cross(p1 - p0, t0) / den;
    float v = cross(p1 - p0, -t1) / den;

    if (u - v > 1e-3f) {
        // linear extension from p0
        vec2 dp = t0 * (u - v);
        return {
            clothoid::segment::from_line(p0, t0, u - v),
            clothoid::segment::from_transition(dp + p0, t0, length, 0.f, curvature),
            clothoid::segment::from_transition(dp + mid, t0 * mat2::rotate(.5f * theta), length, curvature, 0),
        };
    } else if (v - u > 1e-3f) {
        // linear extension from p1
        return {
            clothoid::segment::from_transition(p0, t0, length, 0.f, curvature),
            clothoid::segment::from_transition(mid, t0 * mat2::rotate(.5f * theta), length, curvature, 0),
            clothoid::segment::from_line(p1 - t1 * (v - u), t1, v - u),
        };
    } else {
        // no linear extension
        return {
            clothoid::segment::from_transition(p0, t0, length, 0.f, curvature),
            clothoid::segment::from_transition(mid, t0 * mat2::rotate(.5f * theta), length, curvature, 0),
        };
    }
}

//------------------------------------------------------------------------------
player::player()
    : _usercmd{}
{
    _view.origin = vec2_zero;
    _view.size = vec2(640.f, 480.f);
}

//------------------------------------------------------------------------------
player::~player()
{
}

//------------------------------------------------------------------------------
void player::spawn()
{
}

//------------------------------------------------------------------------------
void player::draw(render::system* renderer, time_value time) const
{
    (void)renderer;
    (void)time;

    vec2 cursor = (_usercmd.cursor - vec2(.5f)) * renderer->view().size + renderer->view().origin;
    float sz = renderer->view().size.length() * (1.f / 512.f);

    clothoid::network::edge_index edge;
    float length;

    if (get_world()->rail_network().get_closest_segment(cursor, 8.f, edge, length)) {
        renderer->draw_box(vec2(sz), cursor, color4(0,1,0,1));
        vec2 spos = get_world()->rail_network().get_segment(edge).evaluate(length);
        renderer->draw_box(vec2(sz), spos, color4(1,0,0,1));
    } else {
        renderer->draw_box(vec2(sz), cursor, color4(1,1,0,1));
    }
}

//------------------------------------------------------------------------------
void player::think()
{
}

//------------------------------------------------------------------------------
vec2 player::get_position(time_value time) const
{
    (void)time;
    return vec2_zero;
}

//------------------------------------------------------------------------------
float player::get_rotation(time_value time) const
{
    (void)time;
    return 0.f;
}

//------------------------------------------------------------------------------
mat3 player::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
}

//------------------------------------------------------------------------------
player_view player::view(time_value time) const
{
    (void)time;
    return _view;
}

//------------------------------------------------------------------------------
void player::set_aspect(float aspect)
{
    _view.size.x = _view.size.y * aspect;
}

//------------------------------------------------------------------------------
void player::update_usercmd(usercmd cmd, time_value time)
{
    constexpr float zoom_speed = 1.f + (1.f / 32.f);
    constexpr float scroll_speed = 1.f;

    float delta_time = (time - _usercmd_time).to_seconds();

    if (!!(_usercmd.buttons & usercmd::button::scroll_up)) {
        _view.origin.y += scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_down)) {
        _view.origin.y -= scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_left)) {
        _view.origin.x -= scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_right)) {
        _view.origin.x += scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_in)) {
        _view.size *= exp(-zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
        _view.size *= exp(zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::pan)) {
        _view.origin -= (cmd.cursor - _usercmd.cursor) * _view.size;
    }

    _usercmd = cmd;
    _usercmd_time = time;

    if (_usercmd.action == usercmd::action::zoom_in) {
        _view.size *= (1.f / zoom_speed);
    } else if (_usercmd.action == usercmd::action::zoom_out) {
        _view.size *= zoom_speed;
    }
}

} // namespace game
