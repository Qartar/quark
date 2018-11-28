// g_aicontroller.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
#include "g_character.h"
#include "g_ship.h"
#include "g_subsystem.h"
#include "g_weapon.h"
#include "r_model.h"

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
    for (auto& ch : _ship->crew()) {
        vec2 goal = vec2_zero;
        do {
            goal = vec2(_random.uniform_real(), _random.uniform_real()) * (_ship->_model->bounds().maxs() - _ship->_model->bounds().mins()) + _ship->_model->bounds().mins();
        } while (_ship->layout().intersect_compartment(goal) == ship_layout::invalid_compartment);
        ch->set_position(_ship, goal, true);
    }
}

//------------------------------------------------------------------------------
void aicontroller::think()
{
    time_value time = get_world()->frametime();

    if (!_ship) {
        return;
    }

    //
    // update crew
    //

    {
        subsystem* medical_bay = nullptr;

        for (auto& ss : _ship->subsystems()) {
            if (ss->info().type == subsystem_type::medical_bay) {
                medical_bay = ss.get();
                break;
            }
        }

        for (auto& ss : _ship->subsystems()) {
            if (ss->damage()) {
                for (auto& ch : _ship->crew()) {
                    // ignore character if low health
                    if (medical_bay && ch->health() < .5f) {
                        continue;
                    }
                    // ignore character if currently healing at medical bay
                    else if (medical_bay && ch->health() < 1.f && medical_bay->compartment() == ch->compartment()) {
                        continue;
                    }
                    // order character to repair the damaged subsystem if idle
                    else if (ch->is_idle()) {
                        ch->repair(ss);
                        break;
                    }
                }
            }
        }

        for (auto& ch : _ship->crew()) {
            // move to medical bay if character is idle and not at full health
            if (medical_bay && ch->is_idle() && ch->health() < 1.f) {
                ch->move(medical_bay->compartment());
            }
            // move to medical bay even if not idle when health is low
            else if (medical_bay && ch->health() < .5f) {
                if (ch->compartment() != medical_bay->compartment()) {
                    if (!medical_bay->damage() && ch->action() != character::action_type::move) {
                        ch->move(medical_bay->compartment());
                    }
                } else if (medical_bay->damage() && ch->action() != character::action_type::repair) {
                    ch->repair(medical_bay);
                }
            }
            // randomly wander if idle
            else if (ch->is_idle() && _random.uniform_real() < .01f) {
                ch->move(narrow_cast<uint16_t>(rand() % _ship->layout().compartments().size()));
            }
        }
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
#if 1
    if (!_ship->is_destroyed()) {
        constexpr float radius = 128.f;
        constexpr float speed = 16.f;
        constexpr float angular_speed = speed / radius;

        float linear_speed = _ship->engines()->maximum_linear_speed();
        vec2 forward = (vec3(1,0,0) * _ship->get_transform()).to_vec2();

        _ship->engines()->set_target_linear_velocity(forward * linear_speed);
        _ship->engines()->set_target_angular_velocity(angular_speed);
    }
#else
        _ship->engines()->set_target_linear_velocity(vec2_zero);
        _ship->engines()->set_target_angular_velocity(0.f);
#endif
    //
    // update weapons
    //
#if 1
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
#endif
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

            // place crew at random places in the ship
            for (auto& ch : _ship->crew()) {
                vec2 goal = vec2_zero;
                do {
                    goal = vec2(_random.uniform_real(), _random.uniform_real()) * (_ship->_model->bounds().maxs() - _ship->_model->bounds().mins()) + _ship->_model->bounds().mins();
                } while (_ship->layout().intersect_compartment(goal) == ship_layout::invalid_compartment);
                ch->set_position(_ship, goal, true);
            }
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
