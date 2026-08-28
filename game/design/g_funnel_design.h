// g_funnel_design.h
//

#pragma once

#include "cm_vector.h"

////////////////////////////////////////////////////////////////////////////////
class lexer;

namespace file {
class stream;
}

//------------------------------------------------------------------------------
namespace game {

class design_manager;

//------------------------------------------------------------------------------
struct funnel_design
{
    static bool parse(lexer& lex, design_manager const& mgr, funnel_design& turret);
    static void print(file::stream& s, funnel_design const& turret);

    string::buffer id;

    std::vector<vec2> inner_outline; //!< Exhaust port outline
    std::vector<vec2> outer_outline; //!< Exterior structure outline
};

} // namespace game
