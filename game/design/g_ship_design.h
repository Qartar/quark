// g_ship_design.h
//

#pragma once

#include <vector>

#include "cm_string.h"
#include "cm_vector.h"
#include "p_compound.h"

#include "design/g_turret_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct ship_design
{
    string::buffer name;

    float length; //!< Overall length in meters
    float beam; //!< Maximum beam in meters
    float displacement; //!< Displacement in kilograms

    struct turret_instance
    {
        vec2 position; //!< Position of the turret on the ship
        float orientation; //!< Default orientation, in radians from ship ahead
        vec2 train_limit; //!< Minimum and maximum train, in radians from default orientation

        turret_design const* design;
    };

    std::vector<turret_instance> turrets;
    std::vector<vec2> hull_outline;
    physics::compound_shape hull_shape;
};

extern const ship_design ship_yamato_battleship;
extern const ship_design ship_iowa_battleship;
extern const ship_design ship_king_george_v_battleship;
extern const ship_design ship_deutschland_cruiser;
extern const ship_design ship_town_cruiser;
extern const ship_design ship_tribal_destroyer;

} // namespace game
