// g_character.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_character.h"
#include "g_subsystem.h"
#include "g_ship.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type character::_type(object::_type);

constexpr string::literal names[] = {
    "aaron",        "alice",        "ash",
    "brian",        "bertha",       "blix",
    "charlie",      "carol",        "cyrix",
    "david",        "dani",         "dash",
    "evan",         "eve",          "erok",
    "frank",        "francie",      "flyx",
    "george",       "gina",         "guuak",
    "henry",        "haley",        "han",
    "ian",          "isabel",       "ix",
    "jack",         "joan",         "jaxon",
    "kevin",        "katrina",      "kyris",
    "larry",        "liz",          "lox",
    "mike",         "mary",         "mazen",
    "neil",         "nancy",        "noz",
    "oscar",        "olivia",       "orlan",
    "pete",         "patty",        "pax",
    "quigley",      "quinn",        "qaam",
    "roger",        "rachel",       "rip",
    "steve",        "sarah",        "shep",
    "tom",          "traci",        "tosh",
    "ulf",          "uma",          "ug",
    "victor",       "violet",       "vok",
    "walter",       "wendy",        "wuax",
    "xavier",       "xena",         "xav",
    "yuri",         "yvette",       "yael",
    "zack",         "zoey",         "zinn",
};

//------------------------------------------------------------------------------
character::character()
    : _health(1.f)
    , _action(action_type::idle)
    , _path_start(0)
    , _path_end(0)
{
}

//------------------------------------------------------------------------------
character::~character()
{
}

//------------------------------------------------------------------------------
void character::think()
{
    auto compartment = this->compartment();

    {
        float atmosphere = compartment == ship_layout::invalid_compartment ? 1.f
                         :  _ship->state().compartments()[compartment].atmosphere;

        // map [0,.5] -> [1, 0]
        float frac = sqrt(clamp(1.f - 2.f * atmosphere, 0.f, 1.f));
        damage(nullptr, (1.f / 30.f) * frac * FRAMETIME.to_seconds());
    }

    if (_health) {
        constexpr float speed = 1.5f;
        vec2 position = get_position();

        while (_path_start < _path_end) {
            vec2 dir = _path[_path_start] - position;
            float len = dir.normalize_length();
            if (len > speed * FRAMETIME.to_seconds()) {
                set_position(position + dir * speed * FRAMETIME.to_seconds());
                break;
            } else {
                set_position(_path[_path_start]);
                ++_path_start;
            }
        }

        action_type effective_action = _action;
        if (is_moving()) {
            effective_action = action_type::move;
        }
        // automatically become idle after reaching destination
        else if (_action == action_type::move) {
            _action = action_type::idle;
        }

        game::subsystem* subsystem = nullptr;
        for (auto& ss : _ship->subsystems()) {
            if (ss->compartment() == compartment) {
                subsystem = ss.get();
                break;
            }
        }

        if (effective_action == action_type::idle) {
            // automatically repair if idle and the subsystem is damaged
            if (subsystem && subsystem->damage()) {
                effective_action = action_type::repair;
            }
            // TODO: automatically operate?

        } else if (effective_action == action_type::operate) {
            // automatically switch to repair if the subsystem is damaged
            if (subsystem && subsystem->damage()) {
                effective_action = action_type::repair;
            }
        }

        if (effective_action == action_type::repair) {
            if (subsystem && subsystem->damage()) {
                subsystem->repair(repair_rate);
            }
            // automatically become idle if the subsystem finishes repairing
            else if (_action == action_type::repair) {
                _action = action_type::idle;
            }
        }
    }
}

//------------------------------------------------------------------------------
void character::damage(object* /*inflictor*/, float amount)
{
    _health = clamp(_health - amount, 0.f, 1.f);
}

//------------------------------------------------------------------------------
void character::set_position(handle<ship> ship, vec2 position, bool teleport/* = false*/)
{
    _ship = ship;
    set_position(position, teleport);
}

//------------------------------------------------------------------------------
uint16_t character::compartment() const
{
    return _ship ? _ship->layout().intersect_compartment(get_position())
                 : ship_layout::invalid_compartment;
}

//------------------------------------------------------------------------------
bool character::is_moving() const
{
    return _path_start < _path_end;
}

//------------------------------------------------------------------------------
bool character::is_repairing(handle<game::subsystem> subsystem) const
{
    if (!subsystem || !subsystem->damage() || subsystem->compartment() != compartment() || !_health || is_moving()) {
        return false;
    }

    switch (_action) {
        case action_type::idle:
        case action_type::repair:
        case action_type::operate:
            return true;
        default:
            return false;
    }
}

//------------------------------------------------------------------------------
bool character::operate(handle<game::subsystem> subsystem)
{
    auto compartment = !subsystem ? ship_layout::invalid_compartment
                                  : subsystem->compartment();

    if (compartment == ship_layout::invalid_compartment) {
        return false;
    }

    if (move(narrow_cast<uint16_t>(compartment))) {
        // keep path but override action type
        _action = action_type::operate;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool character::repair(handle<game::subsystem> subsystem)
{
    auto compartment = !subsystem ? ship_layout::invalid_compartment
                                  : subsystem->compartment();

    if (compartment == ship_layout::invalid_compartment) {
        return false;
    }

    if (move(narrow_cast<uint16_t>(compartment))) {
        // keep path but override action type
        _action = action_type::repair;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool character::move(vec2 goal)
{
    if (!_ship || _ship->layout().intersect_compartment(get_position()) == ship_layout::invalid_compartment) {
        return false;
    }
    _action = action_type::move;
    _path_start = 0;
    _path_end = _ship->layout().find_path(get_position(), goal, .3f, _path, path_size);
    return true;
}

//------------------------------------------------------------------------------
bool character::move(uint16_t compartment)
{
    if (!_ship || compartment == ship_layout::invalid_compartment) {
        return false;
    }

    // already inside the given compartment
    if (compartment == _ship->layout().intersect_compartment(get_position())) {
        _action = action_type::move;
        _path_start = 0;
        _path_end = 0;
        return true;
    }

    // select a random point in the compartment
    vec2 goal;
    {
        auto const& c = _ship->layout().compartments()[compartment];
        vec2 v0 = _ship->layout().vertices()[c.first_vertex];
        float triangle_running_total[64];
        float area = 0;
        for (std::size_t ii = 2, sz = c.num_vertices; ii < sz; ++ii) {
            vec2 v1 = _ship->layout().vertices()[c.first_vertex + ii - 1];
            vec2 v2 = _ship->layout().vertices()[c.first_vertex + ii - 0];
            float triangle_area = 0.5f * (v2 - v1).cross(v1 - v0);
            triangle_running_total[ii - 2] = area + triangle_area;
            area += triangle_area;
        }

        // select a random triangle uniformly by area
        float r = _random.uniform_real() * area;
        std::size_t triangle_index = 0;
        while (triangle_running_total[triangle_index] < r) {
            ++triangle_index;
        }

        // select a random point on the triangle using barycentric coordinates
        {
            vec2 v1 = _ship->layout().vertices()[c.first_vertex + triangle_index + 1];
            vec2 v2 = _ship->layout().vertices()[c.first_vertex + triangle_index + 2];

            float u = _random.uniform_real();
            float v = _random.uniform_real();
            if (u + v < 1.f) {
                goal = v0 * u + v1 * v + v2 * (1.f - u - v);
            } else {
                goal = v0 * (1.f - u) + v1 * (1.f - v) + v2 * (u + v - 1.f);
            }
        }
    }

    // move to a random point in the compartment
    return move(goal);
}

} // namespace game
