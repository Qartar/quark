// g_particles.h
//

#pragma once

#include "cm_expression.h"
#include "cm_parser.h"
#include "cm_random.h"
#include "cm_vector.h"
#include "r_particle.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct particle_effect {

    struct layer {
        string::buffer name;

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

    string::buffer name;
    string::buffer definition;
    std::vector<layer> layers;

    bool parse(string::view definition);
    parser::result<layer> parse_layer(parser::token const*& tokens, parser::token const* end) const;

    bool parse(parser::context& context);
    bool parse_layer(parser::context& context, layer& layer) const;
    bool parse_layer_flags(parser::context& context, render::particle::flag_bits& flags) const;
};

} // namespace game
