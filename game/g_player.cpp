// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "g_navigation.h"
#include "g_ship.h"

#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
player::player()
    : _view({mat4_identity, vec3_zero, vec2(640.0, 480.0)})
    , _usercmd({})
    , _usercmd_time(time_delta::zero)
    , _timescale_time(time_value::zero)
    , _selection_start(vec2_zero)
    , _selection_time(time_delta::zero)
    , _is_selecting(false)
{
    _view.origin = globe::planar_to_surface(vec2_zero);
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
        n[ii] = normalize(v[ii] - v[ii - 1]).cross(-1.0);
    }
    n.front() = normalize(v.front() - v.back()).cross(-1.0);

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
        double speed_in_knots = target->get_linear_velocity().length() * (1.0 / 0.5144447);
        int heading = int(std::round(90.0 - math::rad2deg(globe::heading(target->get_position(time), target->get_rotation(time)).radians())));
        if (heading < 0) {
            heading += 360;
        }
        vec2 text_size = renderer->string_size(va("%s-class", target->design()->name.c_str()));
        vec2 text_offset = renderer->view().origin + 0.49 * renderer->view().size - text_size;
        renderer->draw_string(va("%s-class", target->design()->name.c_str()), text_offset, color4(1,1,1,1));
        renderer->draw_string(va("%.1f kn %d\xb0", speed_in_knots, heading), text_offset - vec2(0,text_size.y), color4(1,1,1,1));
        int rudder = int(std::round(math::rad2deg(target->engines()->get_rudder_angle())));
        renderer->draw_string(va("%d\xb0 rudder", rudder), text_offset - vec2(0,text_size.y*2), color4(1,1,1,1));
        vec3 local_angular_velocity = target->get_angular_velocity() * target->get_rotation(time).inverse();
        int avelocity = int(std::round(math::rad2deg(local_angular_velocity.z*60.0)));
        renderer->draw_string(va("%d\xb0/min", avelocity), text_offset - vec2(0,text_size.y*3), color4(1,1,1,1));

        renderer->draw_string(va("z=%.1f m", globe::altitude(target->get_position(time))), text_offset - vec2(0,text_size.y*4), color4(1,1,1,1));

        // draw slip angle (debug)
#if 0
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
#endif

        // draw hull outline
        if (target == _hover && !_is_selecting) {
            vec2 size = vec2(target->design()->length, target->design()->beam);
            mat4 scale = mat4((size.x + 1.0) / size.x, 0, 0, 0,
                              0, (size.y + 1.0) / size.y, 0, 0,
                              0, 0, 1, 0, 0, 0, 0, 1);

            renderer->draw_outline(
                target->hull_outline(),
                scale * target->get_transform(time),
                color4(1,1,1,1), color4(1,1,1,.5f));
        }
    }

    if (_is_selecting) {
        std::vector<handle<ship>> selection_preview;
        selection_preview = selection_target(_usercmd.cursor);
        draw_selection(renderer, time, selection_preview);
    } else {
        draw_selection(renderer, time, _selection);
    }

    constexpr time_delta fade_time = time_delta::from_seconds(1.5);
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
        } else if (get_world()->timescale() == 27.f) {
            str = ">>>>";
        } else if (get_world()->timescale() == 81.f) {
            str = ">>>>>";
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
            view.origin + offset - 0.5 * (view.size + size),
            color4(.9f * t * t, 1, 1, t));

        renderer->set_view(old_view);
    }
}

//------------------------------------------------------------------------------
void player::draw_selection(render::system* renderer, time_value time, std::vector<handle<ship>> const& selection) const
{
    if (_is_selecting) {
        vec2 start = (_selection_start - vec2(0.5)) * renderer->view().size + renderer->view().origin;
        vec2 cursor = (_usercmd.cursor - vec2(0.5)) * renderer->view().size + renderer->view().origin;
        bounds b = bounds::from_points({start, cursor});
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
        vec2 size = vec2(ship->design()->length, ship->design()->beam);
        mat4 scale = mat4((size.x + 1.0) / size.x, 0, 0, 0,
                          0, (size.y + 1.0) / size.y, 0, 0,
                          0, 0, 1, 0, 0, 0, 0, 1);

        renderer->draw_outline(
            ship->hull_outline(),
            scale * ship->get_transform(time),
            color4(1,1,1,1), color4(1,1,1,.5f));
    }

    // draw order preview
    if (!_is_selecting) {
        vec3 origin = vec3_zero;
        for (auto&& ship : selection) {
            origin += ship->get_position(time);
        }
        origin /= float(selection.size());
        vec3 cursor = _hover ? _hover->get_position(time)
                             : screen_to_world(_usercmd.cursor - vec2(0.5));

        vec2 view_origin = (origin * renderer->view().transform).to_vec2();
        vec2 view_cursor = (cursor * renderer->view().transform).to_vec2();

        renderer->draw_line(view_origin, view_cursor, color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_string(va("%.1f km", 1e-3 * globe::distance(origin, cursor)), 0.5 * (view_origin + view_cursor), color4(1,1,1,1));
        int heading = int(std::round(90.f - math::rad2deg(globe::bearing(origin, cursor).radians())));
        if (heading < 0) {
            heading += 360;
        }
        renderer->draw_string(va("%d\xb0", heading), view_cursor, color4(1,1,1,1));
    }
}

//------------------------------------------------------------------------------
void player::think()
{
}

//------------------------------------------------------------------------------
vec3 player::get_position(time_value time) const
{
    (void)time;
    return vec3_zero;
}

//------------------------------------------------------------------------------
rot3 player::get_rotation(time_value time) const
{
    (void)time;
    return rot3_identity;
}

//------------------------------------------------------------------------------
mat4 player::get_transform(time_value time) const
{
    (void)time;
    return mat4_identity;
}

//------------------------------------------------------------------------------
player_view player::view(time_value time, time_value realtime) const
{
    (void)realtime;

    if (_follow) {
        game::player_view view = _view;
        view.origin = _follow->get_position(time);
        view.transform = globe::surface_inverse_projection(view.origin);
        return view;
    } else {
        game::player_view view = _view;
        view.transform = globe::surface_inverse_projection(_view.origin);
        return view;
    }
}

//------------------------------------------------------------------------------
void player::set_aspect(float aspect)
{
    _view.size.x = _view.size.y * aspect;
}

//------------------------------------------------------------------------------
void player::update_usercmd(usercmd cmd, time_value realtime)
{
    constexpr double zoom_speed = 1.0 + (1.0 / 4.0);
    constexpr double scroll_speed = 1.0;

    double delta_time = (realtime - _usercmd_time).to_seconds();

    if (!!(cmd.buttons & usercmd::button::select)
        && !(_usercmd.buttons & usercmd::button::select)) {
        _is_selecting = true;
        _selection_start = _usercmd.cursor;
    } else if (!(cmd.buttons & usercmd::button::select)
        && !!(_usercmd.buttons & usercmd::button::select)) {
        on_select(_usercmd.cursor);
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
        on_zoom(_view.size * exp(-zoom_speed * delta_time));
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
        on_zoom(_view.size * exp(zoom_speed * delta_time));
    }
    if (!!(_usercmd.buttons & usercmd::button::pan)) {
        on_pan(cmd.cursor);
    }

    _usercmd = cmd;
    _usercmd_time = realtime;

    if (_follow) {
        _view.origin = _follow->get_position();
    }

    vec3 cursor = screen_to_world(_usercmd.cursor - vec2(0.5));
    _hover = hover_target(_usercmd.cursor);

    if (_usercmd.action == usercmd::action::zoom_in) {
        on_zoom(_view.size * (1.0 / zoom_speed));
    } else if (_usercmd.action == usercmd::action::zoom_out) {
        on_zoom(_view.size * zoom_speed);
    } else if (_usercmd.action == usercmd::action::move) {
        if (_selection.size()) {
            vec3 origin = vec3_zero;
            for (auto&& ship : _selection) {
                origin += ship->get_position();
            }
            origin /= double(_selection.size());
            rot2 heading = globe::bearing(origin, cursor);
            // Round to the nearest whole degree
            double rounded = std::round(math::rad2deg(heading.radians()));
            heading = rot2(math::deg2rad(rounded));
            for (auto&& ship : _selection) {
                ship->navigation()->set_heading(heading);
            }
        }
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
handle<ship> player::hover_target(vec2 cursor) const
{
    game::object* obj = get_world()->point_query(screen_to_world(cursor - vec2(0.5)));
    return obj ? obj->cast<ship>() : nullptr;
}

//------------------------------------------------------------------------------
std::vector<handle<ship>> player::selection_target(vec2 cursor) const
{
    game::object* objects[256];
    mat4 transform = globe::surface_inverse_projection(_view.origin);
    bounds b = bounds::from_points({
        (_selection_start - vec2(0.5)) * _view.size,
        (cursor - vec2(0.5)) * _view.size});
    std::size_t num_objects = get_world()->bounds_query(b, transform, objects);
    std::vector<handle<ship>> selection;
    selection.reserve(num_objects);
    for (std::size_t ii = 0; ii < num_objects; ++ii) {
        // Filter out everything that's not a ship
        if (!objects[ii]->is_type<ship>()) {
            continue;
        }
        // Filter out objects on the opposite side of the globe
        if (dot(_view.origin, objects[ii]->get_position()) < 0.0) {
            continue;
        }
        selection.push_back(objects[ii]->cast<ship>());
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

//------------------------------------------------------------------------------
void player::on_pan(vec2 cursor)
{
    if (cursor == _usercmd.cursor) {
        return;
    }

    vec3 surface;
    mat4 transform;

    if (_follow) {
        surface = _follow->get_position();
        transform = globe::surface_projection(surface);
    } else {
        surface = _view.origin;
        transform = globe::surface_projection(surface);
    }

    vec3 cursor_start = vec3((_usercmd.cursor - vec2(0.5)) * _view.size) * transform;
    double t1 = globe::intersect(cursor_start, -surface);

    if (t1 == DBL_MAX) {
        return;
    }

    vec3 surface_start = cursor_start - surface * t1;
    vec3 view_origin = _view.origin;

    // Dynamic epsilon based on view size
    double epsilon = length_sqr(_view.size) * square(1e-5f);

    // Iteratively find a new view origin so that the cursor stays at the same point
    // in world space. An analytical solution to this problem likely exists.
    for (std::size_t ii = 0; ii < 32; ++ii) {
        vec3 cursor_end = vec3((cursor - vec2(0.5)) * _view.size) * transform;
        double t2 = globe::intersect(cursor_end, -surface);

        if (t2 == DBL_MAX) {
            return;
        }

        vec3 delta = cursor_end - surface * t2 - surface_start;
        if (length_sqr(delta) < epsilon) {
            break;
        }
        view_origin -= delta;
        surface = view_origin;
        transform = globe::surface_projection(surface);
    }

    _view.origin = view_origin;
    _follow = nullptr;
}

//------------------------------------------------------------------------------
void player::on_zoom(vec2 view_size)
{
    if (_follow) {
        _view.size = view_size;
        return;
    }

    vec3 surface = _view.origin;
    mat4 transform = globe::surface_projection(surface);

    vec3 cursor_start = vec3((_usercmd.cursor - vec2(0.5)) * _view.size) * transform;
    double t1 = globe::intersect(cursor_start, -surface);

    if (t1 == DBL_MAX) {
        return;
    }

    vec3 surface_start = cursor_start - surface * t1;
    vec3 view_origin = _view.origin;

    // Dynamic epsilon based on view size
    double epsilon = length_sqr(_view.size) * square(1e-5f);

    // Iteratively find a new view origin so that the cursor stays at the same point
    // in world space. An analytical solution to this problem likely exists.
    for (std::size_t ii = 0; ii < 32; ++ii) {
        vec3 cursor_end = vec3((_usercmd.cursor - vec2(0.5)) * view_size) * transform;
        double t2 = globe::intersect(cursor_end, -surface);

        if (t2 == DBL_MAX) {
            return;
        }

        vec3 delta = cursor_end - surface * t2 - surface_start;
        if (length_sqr(delta) < epsilon) {
            break;
        }
        view_origin -= delta;
        surface = view_origin;
        transform = globe::surface_projection(surface);
    }

    _view.origin = view_origin;
    _view.size = view_size;
}

//------------------------------------------------------------------------------
vec3 player::screen_to_world(vec2 v) const
{
    mat4 transform = globe::surface_projection(_view.origin);
    vec3 start = vec3(v * _view.size) * transform;
    double t = globe::intersect(start, -_view.origin);

    if (t < DBL_MAX) {
        return start - t * _view.origin;
    } else {
        return vec3_zero;
    }
}

//------------------------------------------------------------------------------
vec2 player::world_to_screen(vec3 v) const
{
    mat4 transform = globe::surface_inverse_projection(_view.origin);
    return (v * transform).to_vec2();
}

} // namespace game
