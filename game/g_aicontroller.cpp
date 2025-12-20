// g_aicontroller.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
#include "g_navigation.h"
#include "g_ship.h"
#include "g_subsystem.h"
#include "g_weapon.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type aicontroller::_type(object::_type);

//------------------------------------------------------------------------------
aicontroller::aicontroller(ship* target)
    : _ship(target)
    , _destroyed_time(time_value::max)
{}

//------------------------------------------------------------------------------
aicontroller::~aicontroller()
{}

//------------------------------------------------------------------------------
void aicontroller::spawn()
{
}

//------------------------------------------------------------------------------
void aicontroller::think()
{
    time_value time = get_world()->frametime();

    if (!_ship) {
        return;
    }

    //
    // update subsystem power
    //

    if (_ship->reactor()) {
        float maximum_power = _ship->reactor()->maximum_power() - _ship->reactor()->damage();
        float current_power = static_cast<float>(_ship->reactor()->current_power());
        float overload = current_power - maximum_power;

        // Deactivate subsystems if reactor is overloaded
        for (std::size_t ii = _ship->subsystems().size(); ii > 0 && overload > 0.f; --ii) {
            auto& subsystem = _ship->subsystems()[ii - 1];
            if (subsystem->info().type == subsystem_type::reactor) {
                continue;
            }
            while (subsystem->desired_power() && overload > 0.f) {
                subsystem->decrease_power(1);
                overload -= 1.f;
            }
        }

        // Reactivate subsystems if reactor is underloaded
        for (std::size_t ii = 0; ii < _ship->subsystems().size() && overload < -1.f; ++ii) {
            auto& subsystem = _ship->subsystems()[ii];
            if (subsystem->info().type == subsystem_type::reactor) {
                continue;
            }
            while (subsystem->desired_power() < subsystem->maximum_power() && overload < -1.f) {
                subsystem->increase_power(1);
                overload += 1.f;
            }
        }
    }

    //
    // update navigation
    //

    {
        constexpr float radius = 128.f;
        vec2 target = vec2(radius, -radius) * _ship->get_transform();
        _ship->navigation()->set_waypoint(target);
    }
}

//------------------------------------------------------------------------------
vec2 aicontroller::get_position(time_value time) const
{
    return _ship ? _ship->get_position(time) : vec2_zero;
}

//------------------------------------------------------------------------------
rot2 aicontroller::get_rotation(time_value time) const
{
    return _ship ? _ship->get_rotation(time) : rot2_identity;
}

//------------------------------------------------------------------------------
mat3 aicontroller::get_transform(time_value time) const
{
    return _ship ? _ship->get_transform(time) : mat3_identity;
}

} // namespace game
