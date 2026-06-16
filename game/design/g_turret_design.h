// g_turret_design.h
//

#pragma once

#include "cm_vector.h"

#include "design/g_gun_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct turret_design
{
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

extern const turret_design turret_yamato_46cm;
extern const turret_design turret_yamato_15_5cm;
extern const turret_design turret_fuso_36cm;
extern const turret_design turret_fuso_36cm_rf;
extern const turret_design turret_iowa_16in;
extern const turret_design turret_north_carolina_16in;
extern const turret_design turret_north_carolina_5in;
extern const turret_design turret_kgv_14in_quad;
extern const turret_design turret_kgv_14in_twin;
extern const turret_design turret_kgv_5_25in;
extern const turret_design turret_richelieu_380mm;
extern const turret_design turret_richelieu_152mm;
extern const turret_design turret_bismarck_38cm;
extern const turret_design turret_bismarck_15cm_rf;
extern const turret_design turret_bismarck_15cm;
extern const turret_design turret_littorio_381mm;
extern const turret_design turret_littorio_152mm;
extern const turret_design turret_deutschland_28cm;
extern const turret_design turret_town_6in;
extern const turret_design turret_tribal_4_7in;

} // namespace game
