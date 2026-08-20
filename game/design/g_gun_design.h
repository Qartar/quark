// g_gun_design.h
//

#pragma once

#include "cm_string.h"

////////////////////////////////////////////////////////////////////////////////
class lexer;

namespace file {
class stream;
}

//------------------------------------------------------------------------------
namespace game {

class design_manager;

//------------------------------------------------------------------------------
struct gun_design
{
    static bool parse(lexer& lex, design_manager const& mgr, gun_design& gun);
    static void print(file::stream& s, gun_design const& gun);

    string::buffer id;
    string::buffer name;

    float caliber; //!< Internal diameter of gun barrels
    float length; //!< Length of gun barrels

    float shell_mass; //!< Mass of projectile
    float shell_velocity; //!< Muzzle velocity of projectile

    float shell_coefficient; //!< Ballistic coefficient of projectile

    std::vector<vec2> outline;
};

} // namespace game
