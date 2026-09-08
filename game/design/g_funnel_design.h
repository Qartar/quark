// g_funnel_design.h
//

#pragma once

#include "cm_vector.h"
#include "r_outline.h"

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

    render::outline inner_outline; //!< Exhaust port outline
    render::outline outer_outline; //!< Exterior structure outline
};

} // namespace game
