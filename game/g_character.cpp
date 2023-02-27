// g_character.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_character.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type character::_type(object::_type);

const string::literal names[] = {
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
    : _hungry(0.f)
    , _thirsty(0.f)
    , _drowsy(0.f)
    , _tired(0.f)
    , _cold(0.f)
    , _hot(0.f)
    , _wet(0.f)
{
    recalculate_rates();
}

//------------------------------------------------------------------------------
character::~character()
{
}

//------------------------------------------------------------------------------
void character::think()
{
    update_needs();

    update_health();

    update_tasks();

    update_planning();

    update_movement();

    update_action();
}

//------------------------------------------------------------------------------
void character::recalculate_rates()
{
    _hunger_per_tick = FRAMETIME.to_seconds() / SECONDS_PER_DAY.to_seconds();
    _thirst_per_tick = FRAMETIME.to_seconds() / SECONDS_PER_DAY.to_seconds();
    _drowsy_per_tick = FRAMETIME.to_seconds() / SECONDS_PER_DAY.to_seconds();
    _tired_per_tick = -.2f * FRAMETIME.to_seconds() / SECONDS_PER_DAY.to_seconds();

    _resting_drowsy_per_tick = -2.f * FRAMETIME.to_seconds() / SECONDS_PER_DAY.to_seconds();
    _resting_tired_per_tick = -2.f * FRAMETIME.to_seconds() / SECONDS_PER_DAY.to_seconds();
}

//------------------------------------------------------------------------------
void character::update_needs()
{
    _hungry = max(0.f, _hungry + _hunger_per_tick);
    _thirsty = max(0.f, _thirsty + _thirst_per_tick);

    // increase hunger when cold (shivering)
    if (_cold > 0.f) {
        _hungry = max(0.f, _thirsty + _cold * _hunger_per_tick);
    }
    // increase thirst when hot (sweating)
    //  humans can sweat up to 10-14 liters per day
    //  only mammals sweat, and very few primarily sweat for cooling
    if (_hot > 0.f) {
        _thirsty = max(0.f, _thirsty + _hot * _thirst_per_tick);
        _wet = max(0.f, _wet + _hot * _thirst_per_tick);
    }

    if (/*is_sleeping*/false) {
        _drowsy = max(0.f, _drowsy + _resting_drowsy_per_tick);
        _tired = max(0.f, _tired + _resting_tired_per_tick);
    } else {
        _drowsy = max(0.f, _drowsy + _drowsy_per_tick);
        _tired = max(0.f, _tired + _tired_per_tick);
    }
}

//------------------------------------------------------------------------------
void character::update_health()
{
    // starvation
    if (_hungry > 1.f) {
    }

    // dehydration
    if (_thirsty > 1.f) {
    }

    // sleep deprivation
    if (_drowsy > 1.f) {
    }

    // hypothermia
    if (_cold > 1.f) {
    }

    // heat stroke
    if (_hot > 1.f) {
    }
}

//------------------------------------------------------------------------------
void character::update_tasks()
{
}

//------------------------------------------------------------------------------
void character::update_planning()
{
}

//------------------------------------------------------------------------------
void character::update_movement()
{
}

//------------------------------------------------------------------------------
void character::update_action()
{
}

} // namespace game
