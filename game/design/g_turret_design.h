// g_turret_design.h
//

#pragma once

#include "cm_vector.h"

#include "design/g_gun_design.h"

////////////////////////////////////////////////////////////////////////////////
class lexer;

namespace file {
class stream;
}

//------------------------------------------------------------------------------
namespace game {

class design_manager;

//------------------------------------------------------------------------------
struct turret_design
{
    static bool parse(lexer& lex, design_manager const& mgr, turret_design& turret);
    static void print(file::stream& s, turret_design const& turret);

    string::buffer id;

    float radius; //!< Radius of the turret ring
    float train_speed; //!< Angular speed in radians/sec
    float elevation_speed; //!< Angular speed in radians/sec
    vec2 elevation_limit; //!< Minimum and maximum elevation angle, in radians

    static constexpr int max_guns = 4;
    int num_guns; //!< Number of gun barrels
    vec3 position[max_guns];  //!< Position of each gun barrel
    time_delta reload_time; //!< Time to reload all barrels

    gun_design const* gun_design;

    std::vector<vec2> outline;
};

} // namespace game
