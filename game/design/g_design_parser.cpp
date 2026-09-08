// g_design_parser.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_design_parser.h"
#include "r_outline.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
bool check_field(lexer& lex,
                 string::view field_name,
                 time_delta& field_value,
                 bool (*check_units)(lexer&,double&))
{
    double value;
    if (lex.check_token(field_name)
        && lex.expect_token("=")
        && lex.parse(value)
        && check_units(lex, value)
        && lex.expect_token(";")) {
        field_value = time_delta::from_seconds(value);
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool check_field(lexer& lex, string::view field_name, render::outline& outline)
{
    vec2 v;
    std::vector<vec2> verts;

    if (!lex.check_token(field_name) || !lex.expect_token("=") || !lex.expect_token("[")) {
        return false;
    }

    while (!lex.has_error() && !lex.peek_token("]") && lex.parse(v)) {
        verts.push_back(v);
        // Trailing comma is allowed
        if (!lex.check_token(",")) {
            break;
        }
    }

    if (!lex.has_error()
        && lex.expect_token("]")
        && lex.expect_token(";")) {
        outline = render::outline(verts.data(), verts.size());
        return true;
    } else {
        return false;
    }
}

} // namespace game
