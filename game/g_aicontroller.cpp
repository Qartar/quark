// g_aicontroller.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
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

    if (!_ship->is_destroyed()) {
        constexpr float radius = 128.f;
        constexpr float speed = 16.f;
        constexpr float angular_speed = speed / radius;

        float linear_speed = _ship->engines()->maximum_linear_speed();
        vec2 forward = (vec3(1,0,0) * _ship->get_transform()).to_vec2();

        _ship->engines()->set_target_linear_velocity(forward * linear_speed);
        _ship->engines()->set_target_angular_velocity(angular_speed);
    }

    //
    // update weapons
    //

    for (auto& weapon : _ship->weapons()) {
        if (!_ship->is_destroyed() && _random.uniform_real() < .01f) {
            // get a list of all ships in the world
            std::vector<game::ship*> ships;
            for (auto* object : get_world()->objects()) {
                if (object != _ship && object->is_type<ship>()) {
                    if (!static_cast<ship*>(object)->is_destroyed()) {
                        ships.push_back(static_cast<ship*>(object));
                    }
                }
            }

            // select a random target
            if (ships.size()) {
                game::ship* target = ships[_random.uniform_int(ships.size())];
                if (std::holds_alternative<projectile_weapon_info>(weapon->info())
                    || std::holds_alternative<pulse_weapon_info>(weapon->info())) {
                    weapon->attack_point(target, vec2_zero);
                } else if (std::holds_alternative<beam_weapon_info>(weapon->info())) {
                    vec2 v = _random.uniform_nsphere<vec2>();
                    weapon->attack_sweep(target, v * -4.f, v * 4.f);
                }
            }
        }

        if (!_ship->is_destroyed()) {
            // cancel pending attacks on targets that have been destroyed
            if (weapon->target() && weapon->target()->is_type<ship>()) {
                if (static_cast<ship const*>(weapon->target())->is_destroyed()) {
                    weapon->cancel();
                }
            }
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
            _ship = get_world()->spawn<ship>();
            _ship->set_position(vec2(_random.uniform_real(-320.f, 320.f), _random.uniform_real(-240.f, 240.f)), true);
            _ship->set_rotation(_random.uniform_real(2.f * math::pi<float>), true);

        }
    }
}

//------------------------------------------------------------------------------
vec2 aicontroller::get_position(time_value time) const
{
    return _ship ? _ship->get_position(time) : vec2_zero;
}

//------------------------------------------------------------------------------
float aicontroller::get_rotation(time_value time) const
{
    return _ship ? _ship->get_rotation(time) : 0.f;
}

//------------------------------------------------------------------------------
mat3 aicontroller::get_transform(time_value time) const
{
    return _ship ? _ship->get_transform(time) : mat3_identity;
}

} // namespace game
