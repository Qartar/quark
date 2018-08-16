// g_module.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

/*
    Ship Modules

    - Each module has its own maximum power level
    - Effective maximum power level is decreased by damage
    - Active power level determines strength/capability of the module
    - Active power level of the reactor module is equal to the sum of the
        power level of all other modules
    - Reactor power level can exceed maximum power level but will cause damage
        to the reactor module over time proportional to the amount of overload
    - If the reactor is destroyed (brought to full damage) it will go critical
        and destroy the ship, other modules can be repaired from full damage
    - Warp drive and weapons should be expensive enough to be effectively
        mutually exclusive
    - Charging/uncharging modules should take non-trivial time and be detectable
        by other ships with suitable equipment (e.g. sensors)
*/

class ship;

//------------------------------------------------------------------------------
enum class module_type
{
    reactor,
    engines,
    weapons,
    shields,

    // miscellaneous:

    sensors,
    cloaking,

    // needs sectors:

    warp_drive,

    // needs economy:

    cargo_bay,

    // needs crew / compartments:

    security,
    medical_bay,
    armory,
    transporter,
};

//------------------------------------------------------------------------------
struct module_info
{
    module_type type;
    int maximum_power;
};

//------------------------------------------------------------------------------
class module : public object
{
public:
    module(game::ship* owner, module_info info);

    virtual void think() override;

    module_info const& info() const { return _module_info; }

    void damage(float amount);
    float damage() const { return _damage; }
    void repair(float damage_per_second);

    int current_power() const;
    int desired_power() const { return _desired_power; }
    int maximum_power() const { return _module_info.maximum_power; }
    void increase_power(int amount);
    void decrease_power(int amount);

protected:
    module_info _module_info;

    float _damage;

    time_value _damage_time;
    static constexpr time_delta repair_delay = time_delta::from_seconds(0.5f);

    float _current_power;
    int _desired_power;

    static constexpr float power_lambda = .5f; //!< power half-life
    static constexpr float power_epsilon = .1f;
    static constexpr float overload_damage = .1f; //!< damage per second per unit of overcharge
};

} // namespace game