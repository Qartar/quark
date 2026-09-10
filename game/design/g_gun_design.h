// g_gun_design.h
//

#pragma once

#include "cm_string.h"
#include "r_outline.h"
#include <vector>

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

    render::outline shell_outline;

    render::outline outline;

    struct range_data {
        double range;
        double elevation; //!< Gun elevation
        vec2 impact; //!< Impact velocity
        time_delta time; //!< Time of flight
    };

    bool interpolate_range(double d, range_data& r) const;

    std::vector<range_data> range_table;
};

} // namespace game
