// g_particles.h
//

#pragma once

#include "cm_expression.h"
#include "cm_parser.h"
#include "cm_random.h"
#include "cm_vector.h"
#include "r_particle.h"

#include <string>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct particle_effect {

    struct layer {
        std::string name;

        expression::value count;
        expression::value size, size_velocity;
        expression::value position[2], velocity[2], acceleration[2];
        expression::value drag;
        expression::value color[4], color_velocity[4];

        render::particle::flag_bits flags;

        int evaluate_count(random& r,
                           vec2 position,
                           vec2 velocity,
                           vec2 direction,
                           float strength) const;
        void evaluate(render::particle& p,
                      random& r,
                      vec2 position,
                      vec2 velocity,
                      vec2 direction,
                      float strength,
                      float index) const;

        expression expr;
    };

    std::string name;
    std::string definition;
    std::vector<layer> layers;

    bool parse(std::string definition);
    parser::result<layer> parse_layer(parser::token const*& tokens, parser::token const* end) const;
};

} // namespace game
