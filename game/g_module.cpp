// g_module.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_module.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
module::module(game::ship* owner, module_info info)
    : object(object_type::module, owner)
    , _module_info(info)
    , _damage(0)
    , _damage_time(time_value::zero)
    , _current_power(static_cast<float>(info.maximum_power))
    , _desired_power(info.maximum_power)
{}

//------------------------------------------------------------------------------
void module::think()
{
    if (_module_info.type == module_type::reactor) {
        ship const* owner = static_cast<ship const*>(_owner.get());
        _current_power = 0.f;
        _desired_power = 0;
        for (auto const* module : owner->modules()) {
            if (module->info().type != module_type::reactor) {
                _current_power += module->_current_power;
                _desired_power += module->_desired_power;
            }
        }

        if (_current_power > _module_info.maximum_power - _damage) {
            float overload = _current_power - (_module_info.maximum_power - _damage);
            damage(overload * overload_damage * FRAMETIME.to_seconds());
        }
    } else {
        float decay_coeff = -std::expm1(-math::ln2<float> * FRAMETIME.to_seconds() / power_lambda);
        _current_power += (_desired_power - _damage - _current_power) * decay_coeff;
    }
}

//------------------------------------------------------------------------------
void module::damage(float amount)
{
    _damage_time = get_world()->frametime();
    _damage = std::min<float>(_damage + amount, static_cast<float>(_module_info.maximum_power));
}

//------------------------------------------------------------------------------
void module::repair(float damage_per_second)
{
    assert(damage_per_second >= 0.f);
    if (get_world()->frametime() - _damage_time > repair_delay) {
        float delta = damage_per_second * FRAMETIME.to_seconds();
        _damage = std::max(0.f, _damage - delta);
    }
}

//------------------------------------------------------------------------------
int module::current_power() const
{
    return static_cast<int>(std::floor(_current_power + power_epsilon));
}

//------------------------------------------------------------------------------
void module::increase_power(int amount)
{
    _desired_power += amount;
}

//------------------------------------------------------------------------------
void module::decrease_power(int amount)
{
    _desired_power -= amount;
}

} // namespace game
