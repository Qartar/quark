// g_navigation.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_navigation.h"
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
        vec2 current_position = ship->get_position();
        rot2 target_heading = _target_heading;

        double epsilon_sqr = square(2.0 * ship->design()->minimum_turning_radius);
        while (_waypoints.size() && (_waypoints[0] - current_position).length_sqr() < epsilon_sqr) {
            _waypoints.erase(_waypoints.begin());
        }

        if (_waypoints.size()) {
            vec2 direction = normalize(_waypoints[0] - current_position);
            target_heading = rot2(direction.x, direction.y);
        }

        double delta_angle = (target_heading * ship->get_rotation().inverse()).radians();
        double angular_velocity = ship->get_linear_velocity().length() * engines->get_rudder_angle()
            / (ship->design()->rudder_angle * ship->design()->minimum_turning_radius);
        double angular_accel = ship->design()->rudder_speed * angular_velocity;

        if (0.5 * square(angular_velocity) / abs(angular_accel) > abs(delta_angle)) {
            engines->set_rudder_target(0);
        } else {
            engines->set_rudder_target(std::copysign(ship->design()->rudder_angle, -delta_angle));
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
