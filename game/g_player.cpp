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
    , _selection_start(vec2_zero)
    , _selection_time(time_delta::zero)
    , _is_selecting(false)
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
        int rudder = int(std::round(math::rad2deg(target->engines()->get_rudder_angle())));
        renderer->draw_string(va("%d\xb0 rudder", rudder), text_offset - vec2(0,text_size.y*2), color4(1,1,1,1));
        int avelocity = int(std::round(math::rad2deg(target->get_angular_velocity()*60.f)));
        renderer->draw_string(va("%d\xb0/min", avelocity), text_offset - vec2(0,text_size.y*3), color4(1,1,1,1));

        // draw slip angle (debug)
        if (target == _hover && !_is_selecting) {
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
        if (target == _hover && !_is_selecting) {
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

    if (_is_selecting) {
        std::vector<handle<ship>> selection_preview;
        selection_preview = selection_target((_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin);
        draw_selection(renderer, time, selection_preview);
    } else {
        draw_selection(renderer, time, _selection);
    }
}

//------------------------------------------------------------------------------
void player::draw_selection(render::system* renderer, time_value time, std::vector<handle<ship>> const& selection) const
{
    if (_is_selecting) {
        vec2 cursor = (_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin;
        bounds b = bounds::from_points({_selection_start, cursor});
        vec2 p[4] = {
            {b[0][0], b[0][1]},
            {b[1][0], b[0][1]},
            {b[1][0], b[1][1]},
            {b[0][0], b[1][1]},
        };
        renderer->draw_line(p[0], p[1], color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_line(p[1], p[2], color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_line(p[2], p[3], color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_line(p[3], p[0], color4(1,1,1,1), color4(1,1,1,1));
    }

    if (!selection.size()) {
        return;
    }

    // draw selection outlines
    for (auto&& ship : selection) {
        std::vector<vec2> outline = create_outline(ship->design()->hull_outline, 1.f);
        mat3 tx = ship->get_transform(time);
        vec2 v0 = outline[0] * tx;
        for (std::size_t ii = 1; ii < outline.size(); ++ii) {
            vec2 v1 = outline[ii] * tx;
            renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
            v0 = v1;
        }
        vec2 v1 = outline[0] * tx;
        renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
    }

    // draw order preview
    if (!_is_selecting) {
        vec2 origin = vec2_zero;
        for (auto&& ship : selection) {
            origin += ship->get_position(time);
        }
        origin /= float(selection.size());
        vec2 cursor = (_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin;
        if (_hover) {
            cursor = _hover->get_position(time);
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

    if (!!(cmd.buttons & usercmd::button::select)
        && !(_usercmd.buttons & usercmd::button::select)) {
        _is_selecting = true;
        _selection_start = (_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin;
    } else if (!(cmd.buttons & usercmd::button::select)
        && !!(_usercmd.buttons & usercmd::button::select)) {
        on_select((_usercmd.cursor - vec2(.5f)) * _view.size + _view.origin);
    }

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
    } else if (_usercmd.action == usercmd::action::move) {
        if (_selection.size()) {
            vec2 origin = vec2_zero;
            for (auto&& ship : _selection) {
                origin += ship->get_position(time);
            }
            origin /= float(_selection.size());
            vec2 direction = normalize(cursor - origin);
            float heading = std::round(math::rad2deg(rot2(direction.x, direction.y).radians()));
            for (auto&& ship : _selection) {
                ship->navigation()->set_heading(rot2(math::deg2rad(heading)));
            }
        }
    }
}

//------------------------------------------------------------------------------
handle<ship> player::hover_target(vec2 cursor) const
{
    game::object* obj = get_world()->point_query(cursor);
    return obj ? obj->cast<ship>() : nullptr;
}

//------------------------------------------------------------------------------
std::vector<handle<ship>> player::selection_target(vec2 cursor) const
{
    game::object* objects[256];
    bounds b = bounds::from_points({_selection_start, cursor});
    std::size_t num_objects = get_world()->bounds_query(b, objects);
    std::vector<handle<ship>> selection;
    selection.reserve(num_objects);
    for (std::size_t ii = 0; ii < num_objects; ++ii) {
        if (objects[ii]->is_type<ship>()) {
            selection.push_back(objects[ii]->cast<ship>());
        }
    }
    return selection;
}

//------------------------------------------------------------------------------
void player::on_select(vec2 cursor)
{
    std::vector<handle<ship>> selection = selection_target(cursor);
    // Check for double-click to set follow target
    if (_usercmd_time - _selection_time < time_delta::from_milliseconds(400)) {
        if (selection.size()) {
            _follow = selection.front();
        }
        _selection.resize(0);
    } else {
        std::swap(_selection, selection);
        _selection_time = _usercmd_time;
    }
    _is_selecting = false;
}

} // namespace game
