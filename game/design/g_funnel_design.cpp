// g_funnel_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_funnel_design.h"
#include "design/g_design_parser.h"
#include "cm_filesystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
bool funnel_design::parse(lexer& lex, design_manager const&, funnel_design& funnel)
{
    if (!lex.expect_token("{")) {
        return false;
    }

    while (!lex.has_error() && !lex.check_token("}")) {
        if (lex.check_token("inner_outline")) {
            if (!lex.expect_token("=") || !lex.expect_token("[")) {
                return false;
            }
            vec2 v;
            while (!lex.has_error() && !lex.peek_token("]") && lex.parse(v)) {
                funnel.inner_outline.push_back(v);
                // Trailing comma is allowed
                if (!lex.check_token(",")) {
                    break;
                }
            }
            if (!lex.has_error()) {
                lex.expect_token("]");
                lex.expect_token(";");
            }
        } else if (lex.check_token("outer_outline")) {
            if (!lex.expect_token("=") || !lex.expect_token("[")) {
                return false;
            }
            vec2 v;
            while (!lex.has_error() && !lex.peek_token("]") && lex.parse(v)) {
                funnel.outer_outline.push_back(v);
                // Trailing comma is allowed
                if (!lex.check_token(",")) {
                    break;
                }
            }
            if (!lex.has_error()) {
                lex.expect_token("]");
                lex.expect_token(";");
            }
        } else {
            lexer::token t;
            if (lex.expect_any_token(t)) {
                lex.set_error(t, "unrecognized field '%.*s'", int(t.end - t.begin), t.begin);
            }
            return false;
        }
    }

    if (!lex.has_error()) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
void funnel_design::print(file::stream& s, funnel_design const& funnel)
{
    s.printf("%s = {\n", funnel.id.c_str());
    s.printf("    inner_outline = [\n");
    for (std::size_t ii = 0; ii < funnel.inner_outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", funnel.inner_outline[ii].x, funnel.inner_outline[ii].y);
    }
    s.printf("    ];\n");
    s.printf("    outer_outline = [\n");
    for (std::size_t ii = 0; ii < funnel.outer_outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", funnel.outer_outline[ii].x, funnel.outer_outline[ii].y);
    }
    s.printf("    ];\n");
    s.printf("};\n");
    s.close();
}

} // namespace game
