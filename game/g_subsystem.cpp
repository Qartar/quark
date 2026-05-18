// g_subsystem.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_subsystem.h"
#include "g_ship.h"

#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type subsystem::_type(object::_type);

//------------------------------------------------------------------------------
subsystem::subsystem(game::ship* owner)
    : object(owner)
    , _damage(0)
    , _damage_time(time_value::zero)
{}

//------------------------------------------------------------------------------
void subsystem::think()
{
}

//------------------------------------------------------------------------------
void subsystem::damage(object* /*inflictor*/, double amount)
{
    _damage_time = get_world()->frametime();
    _damage += amount;
}

//------------------------------------------------------------------------------
void subsystem::repair(double damage_per_second)
{
    assert(damage_per_second >= 0.0);
    if (get_world()->frametime() - _damage_time > repair_delay) {
        double delta = damage_per_second * FRAMETIME.to_seconds();
        _damage = std::max(0.0, _damage - delta);
    }
}

////////////////////////////////////////////////////////////////////////////////
const object_type engines::_type(subsystem::_type);

//------------------------------------------------------------------------------
engines::engines(game::ship* owner)
    : subsystem(owner)
    , _rudder_angle(0)
    , _rudder_target(0)
{
    ship_design const* design = owner->design();

    // Calculate longitudinal drag coefficient by balancing it against maximum power and speed.
    _linear_drag_coefficient[0] = 1e3 * design->power / cube(design->speed);
    // Hand-tuned transverse drag coefficient, could in theory be calculated from slip angle.
    _linear_drag_coefficient[1] = 1e1 * _linear_drag_coefficient[0] * design->length / design->beam;
    // Torque per angular velocity squared due to drag, calculated by hand so
    // there is a 90% chance it is 100% wrong.
    _angular_drag_coefficient = (1.0 / 32.0) * pow(design->length, 4.0) * _linear_drag_coefficient[1];
    // Using moment of inertia of a solid ellipsoid as an approximation, could
    // use rigid body inertia instead but that would also be an approximation
    // since it also assumes uniform mass distribution.
    _inverse_inertia = 5.0 / ((square(design->length) + square(design->beam)) * design->displacement);

    _speed_target = design->speed;
}

//------------------------------------------------------------------------------
void engines::think()
{
    subsystem::think();

    ship_design const* design = _owner->cast<ship>()->design();

    // Update rudder angle
    {
        double rudder_delta = _rudder_target - _rudder_angle;
        if (abs(rudder_delta) > design->rudder_speed * FRAMETIME.to_seconds()) {
            rudder_delta = std::copysign(design->rudder_speed * FRAMETIME.to_seconds(), rudder_delta);
        }
        _rudder_angle += rudder_delta;
    }

    // Update velocity
    {
        vec3 current_axis = vec3(0,0,1) * _owner->get_rotation();
        vec3 current_velocity = _owner->get_linear_velocity();
        vec3 current_direction = vec3(1,0,0) * _owner->get_rotation();
        // Orthogonal velocity components
        vec3 vx = current_direction * dot(current_direction, current_velocity);
        vec3 vy = current_velocity - vx;
        // Calculate drag from linear velocity of ship hull
        vec3 drag_force = -_linear_drag_coefficient[0] * vx * length(vx)
                          -_linear_drag_coefficient[1] * vy * length(vy);
        // Calculate drag from angular velocity of ship hull
        vec3 drag_torque = _angular_drag_coefficient * _owner->get_angular_velocity() * length(_owner->get_angular_velocity());

        // Simplified rudder model: Calculate torque required to match drag torque
        // at target angular velocity and apply directly to forehead.
        double target_curvature = _rudder_angle / (design->rudder_angle * design->minimum_turning_radius);
        double target_angular_velocity = dot(current_velocity, current_direction) * target_curvature;
        vec3 rudder_torque = _angular_drag_coefficient * current_axis * target_angular_velocity * abs(target_angular_velocity);

        vec3 torque = rudder_torque - drag_torque;

        vec3 rudder_offset = current_direction * design->length * -0.45; // FIXME: add to design
        vec3 rudder_direction = current_direction * rot3(current_axis, _rudder_angle);
        vec3 rudder_normal = rudder_direction.cross(current_axis);

        // Calculate force imparted by rudder
        vec3 rudder_force = rudder_normal * torque / (rudder_offset.length() * cos(_rudder_angle));

        // Apply drag
        current_velocity += (rudder_force + drag_force) / design->displacement * FRAMETIME.to_seconds();

        // Apply power
        double current_speed = current_velocity.length();
        if (current_speed < _speed_target) {
            double speed_delta = 1e3 * design->power / (design->speed * design->displacement) * FRAMETIME.to_seconds();
            if (current_speed + speed_delta > _speed_target) {
                speed_delta = _speed_target - current_speed;
            }
            current_velocity += current_direction * speed_delta;
        }

        vec3 angular_velocity = _owner->get_angular_velocity();
        angular_velocity += torque * _inverse_inertia * FRAMETIME.to_seconds();
        // project velocity onto local plane
        angular_velocity = current_axis * dot(angular_velocity, current_axis);
        current_velocity -= current_axis * dot(current_velocity, current_axis);
        _owner->set_linear_velocity(current_velocity);
        _owner->set_angular_velocity(angular_velocity);

        // Even if we project velocity onto the local plane every frame we will
        // drift in the local-z direction due the plane rotating beneath us.
        // TODO: move this to post-physics update so that we're always on plane.
        {
            vec3 vertical = -globe::gravity_normal(_owner->get_position());
            // Reproject position
            _owner->set_position(_owner->get_position() - vertical * globe::altitude(_owner->get_position()));
            // Reproject rotation
            mat3 rotation = mat3(_owner->get_rotation());
            rotation[0] = normalize(rotation[0] - vertical * dot(rotation[0], vertical));
            rotation[1] = normalize(rotation[1] - vertical * dot(rotation[1], vertical));
            rotation[2] = cross(rotation[0], rotation[1]);
            _owner->set_rotation(rotation.to_rotation());
        }
    }
}

} // namespace game
