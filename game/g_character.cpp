// g_character.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_character.h"
#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const std::string names[] = {
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
    : object(object_type::character)
    , _name(names[rand() % countof(names)])
    , _health(1.f)
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
}

//------------------------------------------------------------------------------
void character::damage(object* /*inflictor*/, float amount)
{
    _health = max(0.f, _health - amount);
}

} // namespace game
