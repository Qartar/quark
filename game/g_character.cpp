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
    "aaron",        "alice",        "ash",          "алексе́й",      "αλέξιος",      "عباد",
    "brian",        "bertha",       "blix",         "богдан",                       "بشار",
    "charlie",      "carol",        "cyrix",                        "χάρις",
    "david",        "dani",         "dash",         "дми́трий",      "δαμιανός",     "داود",
    "evan",         "eve",          "erok",                         "ηλίας",
    "frank",        "francie",      "flyx",         "фёдор",                        "فيصل",
    "george",       "gina",         "guuak",        "георгий",      "γιώργος",      "غفار",
    "henry",        "haley",        "han",                          "ομηρος",       "حامد",
    "ian",          "isabel",       "ix",           "игорь",        "ιωάννης",      "اسحاق",
    "jack",         "joan",         "jaxon",                        "ιάσων",        "جعفر",
    "kevin",        "katrina",      "kyris",        "кирилл",       "κόκος",        "خالد",
    "larry",        "liz",          "lox",                          "λουκας",       "لطيف",
    "mike",         "mary",         "mazen",        "максим",       "μάρκος",       "معاذ",
    "neil",         "nancy",        "noz",          "никола",       "νίκος",        "نسیم‎",
    "oscar",        "olivia",       "orlan",        "олег",         "ορέστης",      "عمر",
    "pete",         "patty",        "pax",          "павел",        "πάρις",
    "quigley",      "quinn",        "qaam",                                         "قاسم",
    "roger",        "rachel",       "rip",          "роман",                        "رشيد",
    "steve",        "sarah",        "shep",         "саша",         "σίμων",        "سمير",
    "tom",          "traci",        "tosh",                         "θάνος",        "طارق",
    "ulf",          "uma",          "ug",                                           "عثمان",
    "victor",       "violet",       "vok",          "вадим",        "βασίλης",
    "walter",       "wendy",        "wuax",                                         "وليد",
    "xavier",       "xena",         "xav",
    "yuri",         "yvette",       "yael",         "яков",         "γιάννης",      "يوسف",
    "zack",         "zoey",         "zinn",         "зиновий",      "ζωιλος",       "زيد",
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
void character::spawn()
{
    object::spawn();

    _name = string::buffer(names[_random.uniform_int(countof(names))]);
}

//------------------------------------------------------------------------------
void character::draw(render::system* renderer, time_value time) const
{
    mat3 tx = _ship ? _ship->get_transform(time) : mat3_identity;

    {
        color4 color = _health > 0.f ? color4(.4f,.5f,.6f,1.f) : color4(.6f,.5f,.4f,1.f);
        renderer->draw_arc(get_position(time) * tx, .25f, .15f, -math::pi<float>, math::pi<float>, color);
    }

    if (_path_end > _path_start) {
        color4 color;
        switch(_action) {
            case action_type::move:
                color = color4(0.f,.5f,0.f,1.f);
                break;
            case action_type::repair:
                color = color4(1.f,.5f,0.f,1.f);
                break;
            case action_type::operate:
                color = color4(0.f,.5f,.5f,1.f);
                break;
            default:
                color = color4(1.f,1.f,1.f,1.f);
                break;
        }
        vec2 prev = get_position(time) * tx;
        for (std::size_t ii = _path_start; ii < _path_end; ++ii) {
            vec2 next = _path[ii] * tx;
            renderer->draw_line(prev, next, color, color);
            prev = next;
        }
    }
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
            effective_action = action_type::idle;
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
handle<game::subsystem> character::repair_target() const
{
    return _action == action_type::repair ? _target_subsystem : nullptr;
}

//------------------------------------------------------------------------------
uint16_t character::move_target() const
{
    return _action == action_type::move ? _target_compartment : ship_layout::invalid_compartment;
}

//------------------------------------------------------------------------------
bool character::is_idle() const
{
    // FIXME: check for effective actions
    return _action == action_type::idle;
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
    if (!_health) {
        return false;
    }

    auto compartment = !subsystem ? ship_layout::invalid_compartment
                                  : subsystem->compartment();

    if (compartment == ship_layout::invalid_compartment) {
        return false;
    }

    // return true if this action is already in progress
    if (_action == action_type::operate && _target_subsystem == subsystem) {
        return true;
    }

    if (move(compartment)) {
        // keep path but override action type
        _action = action_type::operate;
        _target_subsystem = subsystem;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool character::repair(handle<game::subsystem> subsystem)
{
    if (!_health) {
        return false;
    }

    auto compartment = !subsystem ? ship_layout::invalid_compartment
                                  : subsystem->compartment();

    if (compartment == ship_layout::invalid_compartment) {
        return false;
    }

    // return true if this action is already in progress
    if (_action == action_type::repair && _target_subsystem == subsystem) {
        return true;
    }

    if (move(compartment)) {
        // keep path but override action type
        _action = action_type::repair;
        _target_subsystem = subsystem;
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool character::move(handle<game::subsystem> subsystem)
{
    return subsystem ? move(subsystem->compartment()) : false;
}

//------------------------------------------------------------------------------
bool character::move(vec2 goal)
{
    if (!_health) {
        return false;
    }

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
    if (!_health) {
        return false;
    }

    if (!_ship || compartment == ship_layout::invalid_compartment) {
        return false;
    }

    // return true if this action is already in progress
    if (_action == action_type::move && _target_compartment == compartment) {
        return true;
    }

    // already inside the given compartment
    if (compartment == _ship->layout().intersect_compartment(get_position())) {
        _action = action_type::move;
        _target_compartment = compartment;
        _path_start = 0;
        _path_end = 0;
        return true;
    }

    // select a random point in the compartment
    vec2 goal = _ship->layout().random_point(_random, compartment);

    // move to a random point in the compartment
    if (move(goal)) {
        _target_compartment = compartment;
        return true;
    } else {
        return false;
    }
}

} // namespace game
