// g_navigation.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_navigation.h"
#include "g_formation.h"
#include "g_ship.h"

#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type navigation::_type(subsystem::_type);

//------------------------------------------------------------------------------
navigation::navigation(game::ship* owner)
    : subsystem(owner)
    , _target_speed(0)
    , _target_heading(0)
    , _formation_index(0)
{}

//------------------------------------------------------------------------------
navigation::~navigation() {}

//------------------------------------------------------------------------------
void navigation::spawn()
{
    object::spawn();
}

//------------------------------------------------------------------------------
void navigation::think()
{
    subsystem::think();

    auto ship = _owner->cast<game::ship>();
    auto engines = ship ? ship->engines() : nullptr;

    if (engines) {
        vec3 current_position = ship->get_position();

        double epsilon_sqr = square(2.0 * ship->design()->minimum_turning_radius);
        while (_waypoints.size() && (_waypoints[0] - current_position).length_sqr() < epsilon_sqr) {
            _waypoints.erase(_waypoints.begin());
        }

        double target_speed = ship->design()->speed;

        double delta_angle;
        if (_formation && _formation_index) {
            vec3 target_position = _formation->target_position(_formation_index);
            vec3 target_velocity = _formation->target_velocity(_formation_index);

            // Intercept the formation target position assuming constant velocity
            double intercept_time = max(90.0, length(target_position - current_position) / _formation->target_speed());
            target_position += target_velocity * intercept_time;
            vec3 direction = (target_position - current_position) * ship->get_rotation().inverse();
            delta_angle = std::atan2(direction.y, direction.x);

            // Modulate speed based on distance to formation target position
            vec3 delta_position = (_formation->target_position(_formation_index) - current_position);
            vec3 proj = (delta_position * ship->get_rotation().inverse()) / _formation->target_speed();
            target_speed = _formation->target_speed() + 1e-1 * proj.x / (1.0 + 1e-1 * std::abs(proj.y));
        } else if (_waypoints.size()) {
            vec3 direction = (_waypoints[0] - current_position) * ship->get_rotation().inverse();
            delta_angle = std::atan2(direction.y, direction.x);
        } else {
            rot2 current_heading = globe::heading(current_position, ship->get_rotation());
            delta_angle = (_target_heading * current_heading.inverse()).radians();
        }

        double angular_velocity = ship->get_linear_velocity().length() * engines->get_rudder_angle()
            / (ship->design()->rudder_angle * ship->design()->minimum_turning_radius);
        double angular_accel = ship->design()->rudder_speed * angular_velocity;

        if (0.5 * square(angular_velocity) / abs(angular_accel) > abs(delta_angle)) {
            engines->set_rudder_target(0);
        } else {
            engines->set_rudder_target(std::copysign(ship->design()->rudder_angle, -delta_angle));
        }

        engines->set_speed_target(target_speed);
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
