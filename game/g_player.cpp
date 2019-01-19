// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "g_character.h"
#include "g_shield.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
player::player(ship* target)
    : _ship(target)
    , _move_selection(0)
    , _move_appending(0)
    , _crew_selection(0)
    , _weapon_selection(0)
    , _cursor_selection(0)
    , _destroyed_time(time_value::max)
{
    _view.origin = _ship ? _ship->get_position() : vec2_zero;
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
    if (!_ship || _ship->is_destroyed()) {
        return;
    }

    vec2 pos = get_position(time);

    for (std::size_t ii = 0, sz = _waypoints.size(); ii < sz; ++ii) {
        vec2 next = _waypoints[ii];
        renderer->draw_line(pos, next, color4(0,1,0,.5f), color4(0,1,0,.5f));
        pos = next;
    }

    {
        vec2 scale = renderer->view().size / vec2(640,480);
        vec2 origin = renderer->view().origin - vec2(290, 220) * scale;
        vec2 box_size = vec2(36,16) * scale;
        vec2 sel_size = box_size * vec2(1, .5f);
        mat3 transform = mat3(scale.x, 0, 0,
                              0, scale.y, 0,
                              origin.x, origin.y, 1);

        if (!_ship->engines()->current_power()) {
            renderer->draw_box(box_size, vec2(0,0) * transform, color4(.5f,.5f,.5f,1));
        } else if (_waypoints.size()) {
            renderer->draw_box(box_size, vec2(0,0) * transform, color4(1.f,.8f,.2f,1));
        } else {
            renderer->draw_box(box_size, vec2(0,0) * transform, color4(1.f,1.f,1.f,1));
        }

        if (_move_selection) {
            renderer->draw_box(sel_size, vec2(0,16) * transform, color4(.4f,1.f,.2f,1));
        }

        for (std::size_t ii = 0, sz = _ship->weapons().size(); ii < sz; ++ii) {
            if (_ship->weapons()[ii]->current_power() < _ship->weapons()[ii]->maximum_power()) {
                renderer->draw_box(box_size, vec2(40.f * (ii + 1), 0) * transform, color4(.5f,.5f,.5f,1));
            } else if (_ship->weapons()[ii]->is_attacking()) {
                renderer->draw_box(box_size, vec2(40.f * (ii + 1), 0) * transform, color4(1.f,.8f,.2f,1));
            } else {
                renderer->draw_box(box_size, vec2(40.f * (ii + 1), 0) * transform, color4(1.f,1.f,1.f,1));
            }
            if (_weapon_selection & (1 << ii)) {
                renderer->draw_box(sel_size, vec2(40.f * (ii + 1), 16) * transform, color4(.4f,1.f,.2f,1));
            }
        }

        if (_cursor_selection) {
            bounds selection = bounds::from_points({
                (_selection_start - vec2(.5f, .5f)) * _view.size + get_position(time),
                (_usercmd.cursor - vec2(.5f, .5f)) * _view.size + get_position(time),
            });

            vec2 points[4] = {
                {selection[0][0], selection[0][1]},
                {selection[1][0], selection[0][1]},
                {selection[0][0], selection[1][1]},
                {selection[1][0], selection[1][1]},
            };

            // draw thin black outline for extra contrast
            renderer->draw_line(1.f * scale[1], points[0], points[2], color4(0,0,0,1), color4(0,0,0,1));
            renderer->draw_line(1.f * scale[1], points[2], points[3], color4(0,0,0,1), color4(0,0,0,1));
            renderer->draw_line(1.f * scale[1], points[3], points[1], color4(0,0,0,1), color4(0,0,0,1));
            renderer->draw_line(1.f * scale[1], points[1], points[0], color4(0,0,0,1), color4(0,0,0,1));

            // draw green selection rectangle
            renderer->draw_line(points[0], points[2], color4(.4f,1.f,.2f,1), color4(.4f,1.f,.2f,1));
            renderer->draw_line(points[2], points[3], color4(.4f,1.f,.2f,1), color4(.4f,1.f,.2f,1));
            renderer->draw_line(points[3], points[1], color4(.4f,1.f,.2f,1), color4(.4f,1.f,.2f,1));
            renderer->draw_line(points[1], points[0], color4(.4f,1.f,.2f,1), color4(.4f,1.f,.2f,1));

            for (auto const& ch : _ship->crew()) {
                vec2 crew_pos = ch->get_position(time) * get_transform(time);
                if (selection.contains(crew_pos)) {
                    renderer->draw_arc(crew_pos, .4f, 1.f * scale[1], 0, 2.f * math::pi<float>, color4(0,0,0,1));
                    renderer->draw_arc(crew_pos, .4f, 0.f, 0, 2.f * math::pi<float>, color4(.4f,1.f,.2f,1));
                }
            }
        }

        if (_crew_selection) {
            for (std::size_t ii = 0, sz = _ship->crew().size(); ii < sz; ++ii) {
                if (_crew_selection & (1 << ii)) {
                    vec2 crew_pos = _ship->crew()[ii]->get_position(time) * get_transform(time);
                    renderer->draw_arc(crew_pos, .4f, 1.f * scale[1], 0, 2.f * math::pi<float>, color4(0,0,0,1));
                    renderer->draw_arc(crew_pos, .4f, 0.f, 0, 2.f * math::pi<float>, color4(.4f,1.f,.2f,1));
                }
            }
        }
    }

#if 0
    if (_waypoints.size()) {
        vec2 local = _waypoints[0] * _ship->get_inverse_transform(lerp);
        float radius = (square(local.x) + square(local.y)) / (2.f * local.y);
        vec2 center = vec2(0, radius) * _ship->get_transform(lerp);
        renderer->draw_arc(center, std::abs(radius), 0.f, 0.f, 2.f * math::pi<float>, color4(0,.5f,1.f,.5f));
    }

    float dr = _ship->get_linear_velocity().length();
    float da = _ship->get_angular_velocity();

    if (dr && da) {
        float radius = dr / da;
        vec2 center = vec2(0, std::copysign(radius, da)) * _ship->get_transform(lerp);
        renderer->draw_arc(center, std::abs(radius), 0.f, 0.f, 2.f * math::pi<float>, color4(1.f,.5f,0,.5f));
    }

    if (da) {
        float radius = _ship->engines()->maximum_linear_speed() / _ship->engines()->maximum_angular_speed();
        vec2 center = vec2(0, std::copysign(radius, da)) * _ship->get_transform(lerp);
        renderer->draw_arc(center, std::abs(radius), 0.f, 0.f, 2.f * math::pi<float>, color4(1.f,0,0,.5f));
    }
#endif
}

//------------------------------------------------------------------------------
void player::think()
{
    time_value time = get_world()->frametime();

    if (!_ship) {
        return;
    }

    _view = view(time);

    //
    // update navigation
    //

    if (!_ship->is_destroyed()) {
        float linear_speed = _ship->engines()->maximum_linear_speed();
        float angular_speed = _ship->engines()->maximum_angular_speed();
        vec2 target_position = _ship->get_position();

        while (_waypoints.size() && (_waypoints[0] - _view.origin).length_sqr() < square(32.f)) {
            _waypoints.erase(_waypoints.begin());
        }

        if (_waypoints.size()) {
            target_position = _waypoints[0];
        }

        if (linear_speed && angular_speed && target_position != _ship->get_position()) {
            vec2 local = target_position * _ship->get_inverse_transform();
            float target_radius = std::abs((square(local.x) + square(local.y)) / (2.f * local.y));
            float turn_radius = linear_speed / angular_speed;
            linear_speed *= min(1.f, target_radius / turn_radius);
            float arc_angle = std::atan2(local.x, target_radius - std::abs(local.y));
            arc_angle = std::fmod(arc_angle + 2.f * math::pi<float>, 2.f * math::pi<float>);
            float arc_length = target_radius * arc_angle;
            float arc_ratio = arc_length / local.length();
            angular_speed *= clamp(turn_radius / target_radius, 1.f - std::exp(100.f * (1.f - arc_ratio)), 1.f);
        }

        if (target_position == _ship->get_position()) {
            _ship->engines()->set_target_velocity(vec2_zero, 0);
        } else {
            vec2 delta_move = target_position - _ship->get_position();
            float delta_angle = std::remainder(std::atan2(delta_move.y, delta_move.x) - _ship->get_rotation(), 2.f * math::pi<float>);
            vec2 move_target = (vec3(linear_speed,0,0) * _ship->get_transform()).to_vec2();

            _ship->engines()->set_target_linear_velocity(move_target);
            _ship->engines()->set_target_angular_velocity(std::copysign(angular_speed, delta_angle));
        }
    }

    //
    // handle respawn
    //

    if (_ship->is_destroyed()) {
        if (_destroyed_time > time) {
            _destroyed_time = time;
        } else if (time - _destroyed_time >= respawn_time) {
            _destroyed_time = time_value::max;
            get_world()->remove(_ship.get());

            // spawn a new ship to replace the destroyed ship's place
            _ship = get_world()->spawn<ship>(ship::by_random(_random));
            _ship->set_position(vec2(_random.uniform_real(-320.f, 320.f), _random.uniform_real(-240.f, 240.f)), true);
            _ship->set_rotation(_random.uniform_real(2.f * math::pi<float>), true);

        }
    }
}

//------------------------------------------------------------------------------
vec2 player::get_position(time_value time) const
{
    return _ship ? _ship->get_position(time) : vec2_zero;
}

//------------------------------------------------------------------------------
float player::get_rotation(time_value time) const
{
    return _ship ? _ship->get_rotation(time) : 0.f;
}

//------------------------------------------------------------------------------
mat3 player::get_transform(time_value time) const
{
    return _ship ? _ship->get_transform(time) : mat3_identity;
}

//------------------------------------------------------------------------------
player_view player::view(time_value time) const
{
    if (_ship) {
        return {_ship->get_position(time), _view.size};
    } else {
        return _view;
    }
}

//------------------------------------------------------------------------------
void player::set_aspect(float aspect)
{
    _view.size.x = _view.size.y * aspect;
}

//------------------------------------------------------------------------------
void player::update_usercmd(usercmd cmd, time_value time)
{
    if (cmd.buttons != _usercmd.buttons) {
        usercmd::button pressed = cmd.buttons & ~_usercmd.buttons;
        if (!!(pressed & usercmd::button::select)) {
            if (_move_selection) {
                vec2 world_cursor = (cmd.cursor - vec2(.5f, .5f)) * _view.size + _ship->get_position(time);
                if (!!(cmd.modifiers & usercmd::modifier::shift)) {
                    _waypoints.push_back(world_cursor);
                    _move_appending = true;
                } else {
                    _waypoints = {world_cursor};
                    _move_selection = false;
                }
            } else if (_weapon_selection) {
                vec2 world_cursor = (cmd.cursor - vec2(.5f, .5f)) * _view.size + _ship->get_position(time);
                if (attack(world_cursor, !!(cmd.modifiers & usercmd::modifier::control))) {
                    _weapon_selection = 0;
                }
            } else {
                _cursor_selection = true;
                _selection_start = cmd.cursor;
            }
        }

        usercmd::button unpressed = _usercmd.buttons & ~cmd.buttons;
        if (!!(unpressed & usercmd::button::select)) {
            if (_cursor_selection) {
                bounds selection = bounds::from_points({
                    (_selection_start - vec2(.5f, .5f)) * _view.size + _ship->get_position(),
                    (_usercmd.cursor - vec2(.5f, .5f)) * _view.size + _ship->get_position(),
                });
                select(selection, !!(cmd.modifiers & usercmd::modifier::shift));
                _cursor_selection = false;
            }
        }
    }

    switch (cmd.action) {
        case usercmd::action::command: {
            vec2 world_cursor = (cmd.cursor - vec2(.5f, .5f)) * _view.size + _view.origin;
            vec2 ship_cursor = world_cursor * _ship->get_inverse_transform();
            command(ship_cursor);
            break;
        }

        case usercmd::action::move:
            _move_selection = !_move_selection;
            _weapon_selection = 0;
            _crew_selection = 0;
            _cursor_selection = false;
            break;

        case usercmd::action::weapon_1:
            toggle_weapon(0, !!(cmd.modifiers & usercmd::modifier::control));
            break;

        case usercmd::action::weapon_2:
            toggle_weapon(1, !!(cmd.modifiers & usercmd::modifier::control));
            break;

        case usercmd::action::weapon_3:
            toggle_weapon(2, !!(cmd.modifiers & usercmd::modifier::control));
            break;

        case usercmd::action::toggle_shield:
            if (!!(cmd.modifiers & usercmd::modifier::control)) {
                handle<shield> shield = _ship->shield();
                if (shield && shield->desired_power() < shield->maximum_power()) {
                    shield->increase_power(shield->maximum_power());
                } else if (shield) {
                    shield->decrease_power(shield->maximum_power());
                }
            }
            break;

        case usercmd::action::zoom_in:
            _view.size /= 1.1f;
            break;

        case usercmd::action::zoom_out:
            _view.size *= 1.1f;
            break;

        default:
            break;
    }

    if (_move_appending && !(cmd.modifiers & usercmd::modifier::shift)) {
        _move_selection = false;
        _move_appending = false;
    }

    _usercmd = cmd;
}

//------------------------------------------------------------------------------
bool player::attack(vec2 position, bool repeat)
{
    ship* target = nullptr;
    for (auto obj : get_world()->objects()) {
        if (!obj->is_type<ship>()) {
            continue;
        }
        mat3 tx = mat3::inverse_transform(obj->get_position(), obj->get_rotation());
        if (!obj->rigid_body().get_shape()->contains_point(position * tx)) {
            continue;
        }
        target = static_cast<ship*>(obj);
        break;
    }

    if (!target) {
        return false;
    }

    for (std::size_t ii = 0, sz = _ship->weapons().size(); ii < sz; ++ii) {
        if (!(_weapon_selection & (1 << ii))) {
            continue;
        }

        handle<weapon> weapon = _ship->weapons()[ii];
        if (std::holds_alternative<projectile_weapon_info>(weapon->info())
            || std::holds_alternative<pulse_weapon_info>(weapon->info())) {
            weapon->attack_point(target, vec2_zero, repeat);
        } else if (std::holds_alternative<beam_weapon_info>(weapon->info())) {
            vec2 v = _random.uniform_nsphere<vec2>();
            weapon->attack_sweep(target, v * -4.f, v * 4.f, repeat);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
void player::select(bounds selection, bool add)
{
    int new_selection = 0;
    mat3 transform = _ship->get_transform();
    for (std::size_t ii = 0, sz = _ship->crew().size(); ii < sz; ++ii) {
        vec2 crew_pos = _ship->crew()[ii]->get_position() * transform;
        if (selection.contains(crew_pos)) {
            new_selection |= (1 << ii);
        }
    }

    if (add) {
        _crew_selection |= new_selection;
    } else {
        _crew_selection = new_selection;
    }
}

//------------------------------------------------------------------------------
void player::command(vec2 position)
{
    for (std::size_t ii = 0, sz = _ship->crew().size(); ii < sz; ++ii) {
        if (_crew_selection & (1 << ii)) {
            _ship->crew()[ii]->move(position);
        }
    }
}

//------------------------------------------------------------------------------
void player::toggle_weapon(int weapon_index, bool toggle_power)
{
    if (_ship->weapons().size() > weapon_index) {
        handle<weapon> weapon = _ship->weapons()[weapon_index];
        if (toggle_power) {
            if (weapon->desired_power() < weapon->maximum_power()) {
                weapon->increase_power(weapon->maximum_power());
            } else {
                weapon->decrease_power(weapon->maximum_power());
            }
        } else {
            _move_selection = false;
            _crew_selection = 0;
            _cursor_selection = false;
            _weapon_selection ^= (1 << weapon_index);
        }
    }
}

} // namespace game
