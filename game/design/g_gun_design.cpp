// g_gun_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_gun_design.h"
#include "design/g_design_parser.h"
#include "cm_filesystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
bool gun_design::parse(lexer& lex, design_manager const&, gun_design& gun)
{
    if (!lex.expect_token("{")) {
        return false;
    }

    while (!lex.has_error() && !lex.check_token("}")) {
        if (check_field(lex, "name", gun.name)) {
        } else if (check_field(lex, "caliber", gun.caliber, units_length)) {
        } else if (check_field(lex, "length", gun.length, units_length)) {
        } else if (check_field(lex, "shell_mass", gun.shell_mass, units_mass)) {
        } else if (check_field(lex, "shell_velocity", gun.shell_velocity, units_speed)) {
        } else if (check_field(lex, "shell_coefficient", gun.shell_coefficient)) {
        } else if (lex.check_token("outline")) {
            if (!lex.expect_token("=") || !lex.expect_token("[")) {
                return false;
            }
            vec2 v;
            while (!lex.has_error() && !lex.peek_token("]") && lex.parse(v)) {
                gun.outline.push_back(v);
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
void gun_design::print(file::stream& s, gun_design const& gun)
{
    s.printf("%s = {\n", gun.id.c_str());
    s.printf("    name = %s;\n", make_literal(gun.name).c_str());
    s.printf("\n");

    s.printf("    caliber = %g m;\n", gun.caliber);
    s.printf("    length = %g m;\n", gun.length);
    s.printf("\n");

    s.printf("    shell_mass = %g kg;\n", gun.shell_mass);
    s.printf("    shell_velocity = %g m/s;\n", gun.shell_velocity);
    s.printf("    shell_coefficient = %g;\n", gun.shell_coefficient);
    s.printf("\n");

    s.printf("    outline = [\n");
    for (std::size_t ii = 0; ii < gun.outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", gun.outline[ii].x, gun.outline[ii].y);
    }
    s.printf("    ];\n");

    s.printf("};\n");
}

} // namespace game
