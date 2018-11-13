// g_subsystem.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_subsystem.h"
#include "g_character.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type subsystem::_type(object::_type);

//------------------------------------------------------------------------------
subsystem::subsystem(game::ship* owner, int compartment, subsystem_info info)
    : object(owner)
    , _subsystem_info(info)
    , _compartment(compartment)
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
        float decay_coeff = -std::expm1(-math::ln2<float> * FRAMETIME.to_seconds() / power_lambda);
        _current_power += (_desired_power - _damage - _current_power) * decay_coeff;
        _current_power = max(0.f, _current_power);

        if (_subsystem_info.type == subsystem_type::medical_bay) {
            static constexpr float heal_rate = 1.f / 30.f;
            ship* owner = static_cast<ship*>(_owner.get());
            for (auto& ch : owner->crew()) {
                int compartment = owner->layout().intersect_compartment(ch->get_position());
                if (compartment == _compartment) {
                    ch->damage(this, -heal_rate * _current_power * FRAMETIME.to_seconds());
                }
            }
        } else if (_subsystem_info.type == subsystem_type::life_support) {
            ship* owner = static_cast<ship*>(_owner.get());
            if (_current_power > .5f) {
                owner->state().recharge((1.0f / 90.f) * (_current_power - .5f));
            } else {
                owner->state().recharge(-1.0f / 90.f);
            }
        }
    }
}

//------------------------------------------------------------------------------
void subsystem::damage(object* inflictor, float amount)
{
    // self-inflicted damage (e.g. overloading) does not reset damage timer
    if (inflictor != this) {
        _damage_time = get_world()->frametime();
    }
    _damage = std::min<float>(_damage + amount, static_cast<float>(_subsystem_info.maximum_power));
}

//------------------------------------------------------------------------------
void subsystem::repair(float damage_per_second)
{
    assert(damage_per_second >= 0.f);
    if (get_world()->frametime() - _damage_time > repair_delay) {
        float delta = damage_per_second * FRAMETIME.to_seconds();
        // raise power level as the subsystem is repaired rather than waiting
        float effective_maximum = _subsystem_info.maximum_power - _damage;
        if (_current_power >= effective_maximum && _current_power < _desired_power) {
            _current_power = std::min<float>(static_cast<float>(_desired_power), effective_maximum + delta);
        }
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
engines::engines(game::ship* owner, int compartment, engines_info info)
    : subsystem(owner, compartment, {subsystem_type::engines, 2})
    , _engines_info(info)
    , _linear_velocity_target(vec2_zero)
    , _angular_velocity_target(0)
{
    _linear_drag_coefficient = std::exp(-math::ln2<float> * FRAMETIME.to_seconds() / _engines_info.linear_drag_lambda);
    _angular_drag_coefficient = std::exp(-math::ln2<float> * FRAMETIME.to_seconds() / _engines_info.angular_drag_lambda);
}

//------------------------------------------------------------------------------
void engines::think()
{
    subsystem::think();

    float power = float(current_power()) / float(maximum_power());
    if (power) {
        //
        // update linear velocity
        //
        {
            vec2 current_velocity = _owner->get_linear_velocity();
            vec2 target_velocity = _linear_velocity_target;
            vec2 maximum_velocity = target_velocity.normalize() * _engines_info.maximum_linear_speed * power;
            // check if target velocity exceeds maximum velocity
            if (target_velocity.dot(target_velocity - maximum_velocity) > 0.f) {
                target_velocity = maximum_velocity;
            }

            vec2 delta_velocity = target_velocity - current_velocity;
            vec2 maximum_acceleration = delta_velocity.normalize() * _engines_info.maximum_linear_accel * power;
            // check if change in velocity exceeds maximum acceleration
            if (delta_velocity.dot(delta_velocity - maximum_acceleration * FRAMETIME.to_seconds()) > 0.f) {
                current_velocity += maximum_acceleration * FRAMETIME.to_seconds();
            } else {
                current_velocity = target_velocity;
            }
            _owner->set_linear_velocity(current_velocity);
        }
        //
        // update angular velocity
        //
        {
            float current_velocity = _owner->get_angular_velocity();
            float target_velocity = _angular_velocity_target;
            float maximum_velocity = std::copysign(_engines_info.maximum_angular_speed * power, target_velocity);
            // check if target velocity exceeds maximum velocity
            if (target_velocity * (target_velocity - maximum_velocity) > 0.f) {
                target_velocity = maximum_velocity;
            }

            float delta_velocity = target_velocity - current_velocity;
            float maximum_acceleration = std::copysign(_engines_info.maximum_angular_accel * power, delta_velocity);
            // check if change in velocity exceeds maximum acceleration
            if (delta_velocity * (delta_velocity - maximum_acceleration * FRAMETIME.to_seconds()) > 0.f) {
                current_velocity += maximum_acceleration * FRAMETIME.to_seconds();
            } else {
                current_velocity = target_velocity;
            }
            _owner->set_angular_velocity(current_velocity);
        }
    }

    if (_owner->get_linear_velocity().length_sqr() > square(_engines_info.maximum_linear_speed * power)) {
        _owner->set_linear_velocity(_owner->get_linear_velocity() * _linear_drag_coefficient);
    }

    if (std::abs(_owner->get_angular_velocity()) > _engines_info.maximum_angular_speed * power) {
        _owner->set_angular_velocity(_owner->get_angular_velocity() * _angular_drag_coefficient);
    }
}

//------------------------------------------------------------------------------
void engines::set_target_velocity(vec2 linear_velocity, float angular_velocity)
{
    set_target_linear_velocity(linear_velocity);
    set_target_angular_velocity(angular_velocity);
}

//------------------------------------------------------------------------------
void engines::set_target_linear_velocity(vec2 linear_velocity)
{
    assert(!isnan(linear_velocity));
    _linear_velocity_target = linear_velocity;
}

//------------------------------------------------------------------------------
void engines::set_target_angular_velocity(float angular_velocity)
{
    assert(!isnan(angular_velocity));
    _angular_velocity_target = angular_velocity;
}

//------------------------------------------------------------------------------
float engines::maximum_linear_speed() const
{
    float power = float(current_power()) / float(maximum_power());
    return _engines_info.maximum_linear_speed * power;
}

//------------------------------------------------------------------------------
float engines::maximum_angular_speed() const
{
    float power = float(current_power()) / float(maximum_power());
    return _engines_info.maximum_angular_speed * power;
}

} // namespace game
