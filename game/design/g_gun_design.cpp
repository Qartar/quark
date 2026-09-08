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
        } else if (check_field(lex, "shell_outline", gun.shell_outline)) {
        } else if (check_field(lex, "outline", gun.outline)) {
        } else {
            lexer::token t;
            if (lex.expect_any_token(t)) {
                lex.set_error(t, "unrecognized field '%.*s'", int(t.end - t.begin), t.begin);
            }
            return false;
        }
    }

    if (!gun.shell_outline.size()) {
        // Generic ballistic capped shell
        vec2 v[9] = {
            vec2(-2.0 * gun.caliber, 0.45 * gun.caliber),
            vec2(-1.5 * gun.caliber, 0.5 * gun.caliber),
            vec2( 0, 0.5 * gun.caliber),
            vec2( 2.4 * gun.caliber, 0.1 * gun.caliber),
            vec2( 2.5 * gun.caliber, 0),
            vec2( 2.4 * gun.caliber,-0.1 * gun.caliber),
            vec2( 0,-0.5 * gun.caliber),
            vec2(-1.5 * gun.caliber,-0.5 * gun.caliber),
            vec2(-2.0 * gun.caliber,-0.45 * gun.caliber),
        };
        gun.shell_outline = render::outline(v, countof(v));
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

    s.printf("    shell_outline = [\n");
    for (std::size_t ii = 0; ii < gun.shell_outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", gun.shell_outline[ii].x, gun.shell_outline[ii].y);
    }
    s.printf("    ];\n");

    s.printf("    outline = [\n");
    for (std::size_t ii = 0; ii < gun.outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", gun.outline[ii].x, gun.outline[ii].y);
    }
    s.printf("    ];\n");

    s.printf("};\n");
}

} // namespace game
