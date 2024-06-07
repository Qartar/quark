// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "g_train.h"

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
    , _usercmd_time(time_value::zero)
    , _timescale_time(time_value::zero)
{
    _view.origin = vec2_zero;
    _view.size = vec2(640.f, 480.f);
    _view.angle = 0.f;
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

    constexpr time_delta fade_time = time_delta::from_seconds(1.5f);
    // FIXME: using _usercmd_time as proxy for realtime
    if (_usercmd_time - _timescale_time < fade_time) {
        string::view str = "";
        if (get_world()->timescale() == 0.f) {
            str = "||";
        } else if (get_world()->timescale() == 1.f) {
            str = ">";
        } else if (get_world()->timescale() == 3.f) {
            str = ">>";
        } else if (get_world()->timescale() == 9.f) {
            str = ">>>";
        }

        float t = 1.f - (_usercmd_time - _timescale_time) / fade_time;

        // Text orientation is fixed in worldspace, create a new view with no
        // rotation so that the text always appears in the top right corner.
        render::view old_view = renderer->view();
        render::view view = old_view;
        view.angle = 0.f;
        renderer->set_view(view);

        vec2 offset = renderer->string_size(">>>");
        vec2 size = renderer->string_size(str);
        renderer->draw_string(
            str,
            view.origin - offset + .5f * (view.size - size),
            color4(.9f * t * t, 1, 1, t));

        renderer->set_view(old_view);
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
player_view player::view(time_value time, time_value realtime) const
{
    constexpr float zoom_speed = 1.f + (1.f / 32.f);
    constexpr float scroll_speed = 1.f;

    player_view view = _view;

    if (_follow) {
        view.origin = _follow->get_position(time);
        view.angle = _follow->get_rotation(time);
    }

    if (realtime > _usercmd_time) {
        float delta_time = (realtime - _usercmd_time).to_seconds();

        if (!!(_usercmd.buttons & usercmd::button::scroll_up)) {
            view.origin.y += scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::scroll_down)) {
            view.origin.y -= scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::scroll_left)) {
            view.origin.x -= scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::scroll_right)) {
            view.origin.x += scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::zoom_in)) {
            view.size *= exp(-zoom_speed * delta_time);
        }
        if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
            view.size *= exp(zoom_speed * delta_time);
        }
    }

    return view;
}

//------------------------------------------------------------------------------
void player::set_aspect(float aspect)
{
    _view.size.x = _view.size.y * aspect;
}

//------------------------------------------------------------------------------
void player::update_usercmd(usercmd cmd, time_value realtime)
{
    constexpr float zoom_speed = 1.f + (1.f / 32.f);
    constexpr float scroll_speed = 1.f;

    float delta_time = (realtime - _usercmd_time).to_seconds();

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
    _usercmd_time = realtime;

    if (_usercmd.action == usercmd::action::zoom_in) {
        _view.size *= (1.f / zoom_speed);
    } else if (_usercmd.action == usercmd::action::zoom_out) {
        _view.size *= zoom_speed;
    } else if (_usercmd.action == usercmd::action::follow) {
        on_follow();
    } else if (_usercmd.action == usercmd::action::speed_up) {
        get_world()->on_speed_up();
        _timescale_time = realtime;
    } else if (_usercmd.action == usercmd::action::speed_down) {
        get_world()->on_speed_down();
        _timescale_time = realtime;
    } else if (_usercmd.action == usercmd::action::pause) {
        get_world()->on_pause();
        _timescale_time = realtime;
    }
}

//------------------------------------------------------------------------------
void player::on_follow()
{
    handle<object> prev = nullptr;

    // cycle through all train objects
    for (auto obj : get_world()->objects()) {
        if (obj->is_type<train>()) {
            if (prev == _follow) {
                _follow = obj;
                break;
            }
            prev = obj;
        }
    }

    // stop following if no more trains
    if (prev == _follow) {
        _view.angle = 0.f;
        _follow = nullptr;
    }
}

} // namespace game
