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
subsystem::subsystem(game::ship* owner, subsystem_info info)
    : object(owner)
    , _subsystem_info(info)
    , _damage(0)
    , _damage_time(time_value::zero)
    , _current_power(static_cast<float>(info.maximum_power))
    , _desired_power(info.maximum_power)
{}

//------------------------------------------------------------------------------
void subsystem::think()
{
    if (_subsystem_info.type == subsystem_type::reactor) {
        ship const* owner = static_cast<ship const*>(_owner.get());
        _current_power = 0.f;
        _desired_power = 0;
        for (auto const& subsystem : owner->subsystems()) {
            if (subsystem->info().type != subsystem_type::reactor) {
                _current_power += subsystem->_current_power;
                _desired_power += subsystem->_desired_power;
            }
        }

        if (_current_power > _subsystem_info.maximum_power - _damage) {
            float overload = _current_power - (_subsystem_info.maximum_power - _damage);
            damage(this, overload * overload_damage * FRAMETIME.to_seconds());
        }
    } else {
        float decay_coeff = -std::expm1(-math::ln2 * FRAMETIME.to_seconds() / power_lambda);
        _current_power += (_desired_power - _damage - _current_power) * decay_coeff;
    }
}

//------------------------------------------------------------------------------
void subsystem::damage(object* /*inflictor*/, float amount)
{
    _damage_time = get_world()->frametime();
    _damage = std::min(_damage + amount, static_cast<float>(_subsystem_info.maximum_power));
}

//------------------------------------------------------------------------------
void subsystem::repair(float damage_per_second)
{
    assert(damage_per_second >= 0.f);
    if (get_world()->frametime() - _damage_time > repair_delay) {
        float delta = damage_per_second * FRAMETIME.to_seconds();
        _damage = std::max(0.f, _damage - delta);
    }
}

//------------------------------------------------------------------------------
int subsystem::current_power() const
{
    return static_cast<int>(_current_power + power_epsilon);
}

//------------------------------------------------------------------------------
void subsystem::increase_power(int amount)
{
    _desired_power += amount;
}

//------------------------------------------------------------------------------
void subsystem::decrease_power(int amount)
{
    _desired_power -= amount;
}

////////////////////////////////////////////////////////////////////////////////
const object_type engines::_type(subsystem::_type);

//------------------------------------------------------------------------------
engines::engines(game::ship* owner)
    : subsystem(owner, {subsystem_type::engines, 2})
    , _rudder_angle(0)
    , _rudder_target(0)
{
    ship_design const* design = owner->design();

    // Calculate longitudinal drag coefficient by balancing it against maximum power and speed.
    _linear_drag_coefficient[0] = 1e3f * design->power / cube(design->speed) / design->displacement;
    // Hand-tuned transverse drag coefficient, could in theory be calculated from slip angle.
    _linear_drag_coefficient[1] = 1e1f * _linear_drag_coefficient[0] * design->length / design->beam;

    _speed_target = design->speed;
}

//------------------------------------------------------------------------------
void engines::think()
{
    subsystem::think();

    ship_design const* design = _owner->cast<ship>()->design();

    // update linear velocity
    {
        vec2 current_velocity = _owner->get_linear_velocity();
        vec2 current_direction = vec2(1,0) * _owner->get_rotation();
        // Orthogonal velocity components
        vec2 vx = current_direction * dot(current_direction, current_velocity);
        vec2 vy = current_velocity - vx;
        // Apply drag along longitudinal and transverse axes
        vec2 drag_force = _linear_drag_coefficient[0] * vx * length(vx)
                        + _linear_drag_coefficient[1] * vy * length(vy);
        current_velocity -= drag_force * FRAMETIME.to_seconds();

        // Apply power
        float current_speed = current_velocity.length();
        if (current_speed < _speed_target) {
            float speed_delta = 1e3f * design->power / (design->speed * design->displacement) * FRAMETIME.to_seconds();
            if (current_speed + speed_delta > _speed_target) {
                speed_delta = _speed_target - current_speed;
            }
            current_velocity += current_direction * speed_delta;
        }

        _owner->set_linear_velocity(current_velocity);
    }

    // update angular velocity
    {
        float rudder_delta = _rudder_target - _rudder_angle;
        if (abs(rudder_delta) > design->rudder_speed * FRAMETIME.to_seconds()) {
            rudder_delta = std::copysign(design->rudder_speed * FRAMETIME.to_seconds(), rudder_delta);
        }
        _rudder_angle += rudder_delta;

        float k = _rudder_angle / (design->rudder_angle * design->minimum_turning_radius);
        // FIXME: doesn't respect current angular velocity (e.g. in a collision)
        // Need a function for angular acceleration based on rudder angle, etc.
        _owner->set_angular_velocity(_owner->get_linear_velocity().length() * k);
    }
}

} // namespace game
