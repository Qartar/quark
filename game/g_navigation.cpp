// g_navigation.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_navigation.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type navigation::_type(subsystem::_type);

//------------------------------------------------------------------------------
navigation::navigation(game::ship* owner)
    : subsystem(owner, {subsystem_type::navigation, 1})
{}

//------------------------------------------------------------------------------
navigation::~navigation() {}

//------------------------------------------------------------------------------
void navigation::spawn()
{
    object::spawn();
}

//------------------------------------------------------------------------------
void navigation::draw(render::system* renderer, time_value time) const
{
    // get a list of all ships in the world
    std::vector<game::ship const*> ships;
    for (auto const* object : get_world()->objects()) {
        if (object != _owner && object->is_type<ship>()) {
            if (!object->cast<ship>()->is_destroyed()) {
                ships.push_back(object->cast<ship>());
            }
        }
    }

    for (auto const* ship : ships) {
        vec2 relative_position = ship->get_position(time) - _owner->get_position(time);
        vec2 relative_velocity = ship->get_linear_velocity() - _owner->get_linear_velocity();

        // find t s.t. |relative_position + t * relative_velocity|^2 < r^2
        float a = relative_velocity.dot(relative_velocity);
        float b = 2.f * relative_position.dot(relative_velocity);
        float c = relative_position.dot(relative_position) - 256.f * 256.f;
        float d = b * b - 4.f * a * c;

        // no collision
        if (d < 0.f) {
            continue;
        }

        float q = -.5f * (b + std::copysign(std::sqrt(d), b));
        auto t12 = std::minmax({q / a, c / q});
        if (t12.second < 0.f) {
            continue;
        }
        float t = std::max(0.f, t12.first);

        renderer->draw_arc(
            _owner->get_position(time) + _owner->get_linear_velocity() * t,
            128.f,
            0.f,
            -math::pi<float>,
            math::pi<float>,
            color4(1,0,0,1));
        renderer->draw_line(
            _owner->get_position(time),
            _owner->get_position(time) +  _owner->get_linear_velocity() * t
            -  _owner->get_linear_velocity().normalize() * 128.f,
            color4(1,0,0,1),
            color4(1,0,0,1));

        rot2 halfangle = rot2(std::asin(256.f / relative_position.length()));
        vec2 dir1 = (relative_position * halfangle).normalize();
        vec2 dir2 = (relative_position / halfangle).normalize();

        //float d2 = 2.f * halfangle.x * halfangle.y;
        //float s = -.5f * relative_velocity.cross(dir1) / d2;
        //vec2 apex = ship->get_linear_velocity() + dir2 * s - relative_position * (1.f / 256.f);
        vec2 apex = ship->get_linear_velocity();// + dir2 * s - relative_position * (1.f / 256.f);

        renderer->draw_line(_owner->get_position(time) + apex, _owner->get_position(time) + apex + dir1 * 64.f, color4(1,1,0,1), color4(1,1,0,0));
        renderer->draw_line(_owner->get_position(time) + apex, _owner->get_position(time) + apex + dir2 * 64.f, color4(1,1,0,1), color4(1,1,0,0));
        renderer->draw_line(_owner->get_position(time), _owner->get_position(time) + _owner->get_linear_velocity(), color4(0,1,0,1), color4(0,1,0,1));
    }
}

//------------------------------------------------------------------------------
void navigation::think()
{
    subsystem::think();

    if (!current_power()) {
        return;
    }

    auto ship = _owner->cast<game::ship>();
    auto engines = ship ? ship->engines() : nullptr;

    if (engines)
    {
        float linear_speed = engines->maximum_linear_speed();
        float angular_speed = engines->maximum_angular_speed();
        vec2 target_position = ship->get_position();

        while (_waypoints.size() && (_waypoints[0] - target_position).length_sqr() < square(32.f)) {
            _waypoints.erase(_waypoints.begin());
        }

        if (_waypoints.size()) {
            target_position = _waypoints[0];
        }

        if (linear_speed && angular_speed && target_position != ship->get_position()) {
            // Target position in ship-local space
            vec2 local = target_position * ship->get_inverse_transform();

            // Radius of circle tangent to the current position and velocity
            // which also contains the target position. This will be infinite
            // if the ship is pointing directly towards the target position.
            float target_radius = std::abs((square(local.x) + square(local.y)) / (2.f * local.y));

            // Decrease linear speed if turn radius is too large
            float turn_radius = linear_speed / angular_speed;
            linear_speed *= min(1.f, target_radius / turn_radius);

            // Angle of arc between current and target position on circle
            float arc_angle = std::atan2(local.x, target_radius - std::abs(local.y));
            arc_angle = std::fmod(arc_angle + 2.f * math::pi<float>, 2.f * math::pi<float>);

            // Length of the arc between current and target position
            float arc_length = target_radius * arc_angle;
            // Ratio of the arc length to straight line distance
            float arc_ratio = arc_length / local.length();

            // Decrease angular speed if turn radius is too small, unless the
            // waypoint is far enough away to use a straight-line course.
            angular_speed *= clamp(turn_radius / target_radius, 1.f - std::exp(100.f * (1.f - arc_ratio)), 1.f);
        }

        if (target_position == ship->get_position()) {
            engines->set_target_velocity(vec2_zero, 0);
        } else {
            vec2 delta_move = target_position * ship->get_inverse_transform();
            vec2 move_target = vec2(linear_speed,0) * ship->get_rotation();

            engines->set_target_velocity(move_target, std::copysign(angular_speed, delta_move.y));
        }
    }
}

//------------------------------------------------------------------------------
void navigation::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void navigation::write_snapshot(network::message& /*message*/) const
{
}

} // namespace game
