// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "g_navigation.h"
#include "g_shield.h"
#include "g_ship.h"

#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
player::player()
    : _view({vec2_zero, vec2(640.f, 480.0f)})
    , _usercmd({})
    , _usercmd_time(time_delta::zero)
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
std::vector<vec2> create_outline(std::vector<vec2> const& v, float d)
{
    std::vector<vec2> n;
    n.resize(v.size());

    for (std::size_t ii = 1; ii < v.size(); ++ii) {
        n[ii] = normalize(v[ii] - v[ii - 1]).cross(-1.f);
    }
    n.front() = normalize(v.front() - v.back()).cross(-1.f);

    std::vector<vec2> o;
    o.resize(v.size());

    for (std::size_t ii = 0; ii + 1 < v.size(); ++ii) {
        o[ii] = v[ii] + normalize(n[ii] + n[ii + 1]) * d;
    }
    o.back() = v.back() + normalize(n.back() + n.front()) * d;

    return o;
}

//------------------------------------------------------------------------------
void player::draw(render::system* renderer, time_value time) const
{
    ship const* target = _hover ? _hover.get()
                       : _selection ? _selection.get()
                       : _follow.get();
    if (target) {
        float speed_in_knots = target->get_linear_velocity().length() * (1.f / 0.5144447f);
        int heading = int(std::round(90.f - math::rad2deg(target->get_rotation().radians())));
        if (heading < 0) {
            heading += 360;
        }
        vec2 text_size = renderer->string_size(va("%s-class", target->design()->name.c_str()));
        vec2 text_offset = _view.origin + .49f * _view.size - text_size;
        renderer->draw_string(va("%s-class", target->design()->name.c_str()), text_offset, color4(1,1,1,1));
        renderer->draw_string(va("%.1f kn %d\xb0", speed_in_knots, heading), text_offset - vec2(0,text_size.y), color4(1,1,1,1));

        // draw slip angle (debug)
        if (target == _hover || target == _selection) {
            renderer->draw_line(
                target->get_position(time),
                target->get_position(time) + target->get_linear_velocity(),
                color4(1,1,1,1),
                color4(1,1,1,1));
            renderer->draw_line(
                target->get_position(time),
                target->get_position(time) + vec2(target->get_linear_velocity().length(),0) * target->get_rotation(),
                color4(1,1,1,1),
                color4(1,1,1,1));
        }

        // draw hull outline
        if (target == _hover || target == _selection) {
            std::vector<vec2> outline = create_outline(target->design()->hull_outline, 1.f);
            mat3 tx = target->get_transform(time);
            vec2 v0 = outline[0] * tx;
            for (std::size_t ii = 1; ii < outline.size(); ++ii) {
                vec2 v1 = outline[ii] * tx;
                renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
                v0 = v1;
            }
            vec2 v1 = outline[0] * tx;
            renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
        }
    }

    if (_selection && _selection != target) {
        // draw hull outline
        {
            std::vector<vec2> outline = create_outline(_selection->design()->hull_outline, 1.f);
            mat3 tx = _selection->get_transform(time);
            vec2 v0 = outline[0] * tx;
            for (std::size_t ii = 1; ii < outline.size(); ++ii) {
                vec2 v1 = outline[ii] * tx;
                renderer->draw_line(v0, v1, color4(.5f,.5f,.5f,1), color4(.5f,.5f,.5f,1));
                v0 = v1;
            }
            vec2 v1 = outline[0] * tx;
            renderer->draw_line(v0, v1, color4(.5f,.5f,.5f,1), color4(.5f,.5f,.5f,1));
        }
    }

    if (_selection) {
        vec2 cursor = (_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin;
        vec2 origin = _selection->get_position(time);
        if (target && target != _selection) {
            cursor = target->get_position(time);
        }

        renderer->draw_line(origin, cursor, color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_string(va("%.1f km", 1e-3f * length(cursor - origin)), .5f * (origin + cursor), color4(1,1,1,1));
        vec2 direction = normalize(cursor - origin);
        int heading = int(std::round(90.f - math::rad2deg(rot2(direction.x, direction.y).radians())));
        if (heading < 0) {
            heading += 360;
        }
        renderer->draw_string(va("%d\xb0", heading), cursor, color4(1,1,1,1));
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
rot2 player::get_rotation(time_value time) const
{
    (void)time;
    return rot2_identity;
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
    constexpr float zoom_speed = 1.f + (1.f / 4.f);
    constexpr float scroll_speed = 1.f;

    float delta_time = (time - _usercmd_time).to_seconds();

    if (!!(_usercmd.buttons & usercmd::button::scroll_up)) {
        _view.origin.y += scroll_speed * _view.size.x * delta_time;
        _follow = nullptr;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_down)) {
        _view.origin.y -= scroll_speed * _view.size.x * delta_time;
        _follow = nullptr;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_left)) {
        _view.origin.x -= scroll_speed * _view.size.x * delta_time;
        _follow = nullptr;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_right)) {
        _view.origin.x += scroll_speed * _view.size.x * delta_time;
        _follow = nullptr;
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_in)) {
        _view.size *= exp(-zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
        _view.size *= exp(zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::pan)) {
        _view.origin -= (cmd.cursor - _usercmd.cursor) * _view.size;
        _follow = nullptr;
    }

    _usercmd = cmd;
    _usercmd_time = time;

    if (_follow) {
        _view.origin = _follow->get_position(time);
    }

    vec2 cursor = (_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin;
    _hover = hover_target(cursor);

    if (_usercmd.action == usercmd::action::zoom_in) {
        _view.size *= (1.f / zoom_speed);
    } else if (_usercmd.action == usercmd::action::zoom_out) {
        _view.size *= zoom_speed;
    } else if (_usercmd.action == usercmd::action::select) {
        if (_selection == _hover) {
            _follow = _hover;
        } else {
            _selection = _hover;
        }
    } else if (_usercmd.action == usercmd::action::move) {
        if (_selection) {
            vec2 direction = normalize(cursor - _selection->get_position(time));
            float heading = std::round(math::rad2deg(rot2(direction.x, direction.y).radians()));
            _selection->navigation()->set_heading(rot2(math::deg2rad(heading)));
        }
    }
}

//------------------------------------------------------------------------------
handle<ship> player::hover_target(vec2 cursor) const
{
    game::object* obj = get_world()->point_query(cursor);
    return obj ? obj->cast<ship>() : nullptr;
}

} // namespace game
