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
        expression::value time;
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

    bool parse(parser::context& context);
    bool parse_layer(parser::context& context, layer& layer) const;
    bool parse_layer_flags(parser::context& context, render::particle::flag_bits& flags) const;
    bool parse_layer_value(parser::context& context, expression_parser& parser, expression::value& value) const;
    bool parse_layer_vector(parser::context& context, expression_parser& parser, expression::value* vector, std::size_t vector_size) const;
    template<std::size_t vector_size> bool parse_layer_vector(parser::context& context, expression_parser& parser, expression::value (&vector)[vector_size]) const {
        return parse_layer_vector(context, parser, vector, vector_size);
    }
    bool parse_local_value(parser::context& context, expression_parser& parser) const;
};

} // namespace game
