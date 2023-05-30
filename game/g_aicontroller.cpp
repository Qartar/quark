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
    // update crew
    //

    {
        struct task {
            game::handle<game::character> character;
            game::handle<game::subsystem> subsystem;
            uint16_t compartment;
            character::action_type action;
            float priority;
            float distance;

            // sort by priority (descending) then by distance (ascending)
            bool operator<(task const& rhs) const {
                return priority > rhs.priority ? true
                     : priority == rhs.priority ? distance < rhs.distance : false;
            }

            bool execute() {
                switch (action) {
                    case character::action_type::repair:
                        return character->repair(compartment);
                    case character::action_type::operate:
                        return character->operate(subsystem);
                    case character::action_type::move:
                    case character::action_type::idle:
                        return character->move(compartment);
                    default:
                        return false;
                }
            }
        };

        // calculate repair priority of the subsystem
        auto const subsystem_priority = [](game::handle<game::subsystem> ss) -> float {
            switch (ss->info().type) {
                case subsystem_type::shields:
                    return 4.f;
                case subsystem_type::reactor:
                    return 3.f;
                case subsystem_type::life_support:
                    return 3.f;
                case subsystem_type::medical_bay:
                    return 2.f;
                default:
                    return 1.f;
            }
        };

        // calculate distance from the given character to the given compartment
        auto const task_distance = [this](uint16_t task_compartment, game::handle<game::character> ch) {
            auto compartment = _ship ? _ship->layout().intersect_compartment(ch->get_position())
                                     : ship_layout::invalid_compartment;

            if (compartment == ship_layout::invalid_compartment) {
                return INFINITY;
            } else if (compartment == task_compartment) {
                return 0.f;
            }

            vec2 path[32]; // FIXME: hard-coded path limit in character
            std::size_t path_len = _ship->layout().find_path(
                ch->get_position(),
                // FIXME: want shortest path to compartment, not path to random
                // point in compartment. this can give different results for
                // compartments with multiple entrypoints even if ignoring the
                // last segment of the path.
                _ship->layout().random_point(_random, task_compartment),
                ch->radius(),
                path,
                countof(path));

            float distance = path_len ? length(ch->get_position() - path[0]) : INFINITY;
            // ignore last segment which is to a random point within the destination compartment
            for (std::size_t ii = 1; ii + 1 < path_len; ++ii) {
                distance += length(path[ii] - path[ii - 1]);
            }
            return distance;
        };

        std::vector<task> tasks;

        game::handle<subsystem> medical_bay = nullptr;

        for (auto& ss : _ship->subsystems()) {
            if (ss->info().type == subsystem_type::medical_bay) {
                medical_bay = ss;
                break;
            }
        }

        for (auto& ch : _ship->crew()) {
            // nothing to do for dead crew members
            if (!ch->health()) {
                continue;
            }

            for (auto& ss : _ship->subsystems()) {
                if (ss->damage()) {
                    float d = task_distance(ss->compartment(), ch);
                    float p = subsystem_priority(ss);
                    // all crew members can repair the same subsystem simultaneously
                    // but the priority for each subsequent crew member decreases.
                    // this creates a unique task for each crew/slot combination
                    // which are then pruned from the task list as each slot is taken.
                    for (std::size_t ii = 0; ii < _ship->crew().size(); ++ii) {
                        tasks.push_back({ch, ss, ss->compartment(), character::action_type::repair, p / (ii + 1), d});
                    }
                }
            }

            if (medical_bay && ch->health() < 1.f) {
                float d = task_distance(medical_bay->compartment(), ch);
                // priority is inversely proportional to remaining health
                tasks.push_back({ch, medical_bay, medical_bay->compartment(), character::action_type::move, 1.f / ch->health(), d});
            }

            for (uint16_t ii = 0, num = narrow_cast<uint16_t>(_ship->state().compartments().size()); ii < num; ++ii) {
                if (_ship->state().compartments()[ii].damage) {
                    float d = task_distance(ii, ch);
                    tasks.push_back({ch, nullptr, ii, character::action_type::move, 1.f, d});
                }
            }
        }

        // simple greedy task assignment

        std::sort(tasks.begin(), tasks.end());

        while (tasks.size()) {
            std::size_t skipped = 0;
            // skip any tasks that fail to execute
            for (; skipped < tasks.size() && !tasks[skipped].execute(); ++skipped)
                ;

            // save executed task information for reference
            task const t = tasks[skipped];

            // starting from the executed task, remove any remaining tasks for
            // the same character or same subsystem/action/priority combination
            std::size_t remaining = 0;
            for (std::size_t ii = skipped + 1; ii < tasks.size(); ++ii) {
                if (tasks[ii].character == t.character
                    || (tasks[ii].subsystem == t.subsystem
                        && tasks[ii].compartment == t.compartment
                        && tasks[ii].action == t.action
                        && tasks[ii].priority == t.priority)) {
                    continue;
                }
                tasks[remaining++] = tasks[ii];
            }
            tasks.resize(remaining);
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
            _ship = get_world()->spawn<ship>(ship::by_random(_random));
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
