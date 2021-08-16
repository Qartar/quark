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
aicontroller::aicontroller(game::ship* target, int team)
    : _ship(target)
    , _team(team)
    , _destroyed_time(time_value::max)
{}

//------------------------------------------------------------------------------
aicontroller::~aicontroller()
{}

//------------------------------------------------------------------------------
void aicontroller::spawn()
{
    object::spawn();
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

    bool need_target = false;
    if (!_ship->is_destroyed()) {
        for (auto& weapon : _ship->weapons()) {
            // cancel pending attacks on targets that have been destroyed
            if (weapon->target() && weapon->target()->is_type<game::ship>()) {
                if (static_cast<game::ship const*>(weapon->target())->is_destroyed()) {
                    weapon->cancel();
                }
            }
            if (!weapon->is_attacking()) {
                need_target = true;
            }
        }
    }

    if (need_target && _random.uniform_real() < .01f) {
        handle<game::ship> best_target = nullptr;
        std::vector<handle<game::ship>> ships;

        for (auto* object : get_world()->objects()) {
            if (object != this && object->is_type<aicontroller>()) {
                if (static_cast<aicontroller*>(object)->team() == _team) {
                    continue;
                }
                handle<game::ship> ship = static_cast<aicontroller*>(object)->ship();
                if (!ship->is_destroyed()) {
                    ships.push_back(ship);
                }
            }
        }

        // sort by distance
        std::sort(ships.begin(), ships.end(), [this](auto const& lhs, auto const& rhs) {
            return length_sqr(lhs->get_position() - _ship->get_position())
                 < length_sqr(rhs->get_position() - _ship->get_position());
        });

        float total_inverse_distance = 0.f;
        for (auto const& ship : ships) {
            total_inverse_distance += 1.f / length_sqr(ship->get_position() - _ship->get_position());
        }

        if (ships.size()) {
#if 0
            float r = _random.uniform_real(total_inverse_distance);
            total_inverse_distance = 0.f;
            for (auto& ship : ships) {
                total_inverse_distance += 1.f / length_sqr(ship->get_position() - _ship->get_position());
                if (r < total_inverse_distance) {
                    best_target = ship;
                    break;
                }
            }
#else
            best_target = ships.front();
#endif

            for (auto& weapon : _ship->weapons()) {
                if (weapon->is_attacking()) {
                    continue;
                }

                if (std::holds_alternative<projectile_weapon_info>(weapon->info())
                    || std::holds_alternative<pulse_weapon_info>(weapon->info())) {
                    weapon->attack_point(best_target.get(), vec2_zero);
                } else if (std::holds_alternative<beam_weapon_info>(weapon->info())) {
                    vec2 v = _random.uniform_nsphere<vec2>();
                    weapon->attack_sweep(best_target.get(), v * -4.f, v * 4.f);
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
            _ship = get_world()->spawn<game::ship>();
            _ship->set_position(vec2(_random.uniform_real(-320.f, 320.f), _random.uniform_real(-240.f, 240.f)), true);
            _ship->set_rotation(rot2(_random.uniform_real(2.f * math::pi<float>)), true);

        }
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
