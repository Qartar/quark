// cm_ballistics.h
//

#pragma once

#include "cm_time.h"
#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
namespace parser {
class text;
} // namespace parser

////////////////////////////////////////////////////////////////////////////////
namespace ballistics {

void solve_ballistic_coefficient_cmd(parser::text const& args);

//------------------------------------------------------------------------------
enum class curve {
    G1, // Ingalls, flat base with 2 calibers ogive
    G2, // Aberdeen J projectile, Short boat-tail, conical nose
    G5, // Short boat-tail, 6.19 calibers tangent ogive
    G6, // Flat base, 6 calibers secant ogive
    G7, // Long boat-tail, 10 calibers secant ogive
    G8, // Flat base, 10 calibers secant ogive
};

//------------------------------------------------------------------------------
void step(vec3& position, vec3& velocity, ballistics::curve curve, float ballistic_coefficient, time_delta dt);

//------------------------------------------------------------------------------
time_delta simulate(vec3& position, vec3& velocity, ballistics::curve curve, float ballistic_coefficient, time_delta timestep);

} // namespace ballistics
