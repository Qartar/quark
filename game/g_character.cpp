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
    if (_health && _subsystem && _subsystem->damage()) {
        _subsystem->repair(repair_rate);
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
    }
}

//------------------------------------------------------------------------------
void character::damage(object* /*inflictor*/, float amount)
{
    _health = max(0.f, _health - amount);
}

//------------------------------------------------------------------------------
void character::set_position(handle<ship> ship, vec2 position, bool teleport/* = false*/)
{
    _ship = ship;
    set_position(position, teleport);
}

//------------------------------------------------------------------------------
void character::set_goal(vec2 goal)
{
    if (!_ship || _ship->layout().intersect_compartment(get_position()) == ship_layout::invalid_compartment) {
        return;
    }
    _path_start = 0;
    _path_end = _ship->layout().find_path(get_position(), goal, .3f, _path, path_size);
}

} // namespace game
