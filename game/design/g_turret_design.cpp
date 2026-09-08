// g_turret_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_turret_design.h"
#include "design/g_design_parser.h"
#include "design/g_gun_design.h"
#include "cm_filesystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
bool turret_design::parse(lexer& lex, design_manager const& mgr, turret_design& turret)
{
    if (!lex.expect_token("{")) {
        return false;
    }

    while (!lex.has_error() && !lex.check_token("}")) {
        if (check_field(lex, "radius", turret.radius, units_length)) {
        } else if (check_field(lex, "train_speed", turret.train_speed, units_angle)) {
        } else if (check_field(lex, "elevation_speed", turret.elevation_speed, units_angle)) {
        } else if (check_field(lex, "elevation_limit", turret.elevation_limit, units_angle)) {
        } else if (check_field(lex, "num_guns", turret.num_guns)) {
        } else if (lex.check_token("position")) {
            if (!lex.expect_token("=") || !lex.expect_token("[")) {
                return false;
            }
            for (int ii = 0; !lex.has_error() && !lex.peek_token("]"); ++ii) {
                if (ii >= turret_design::max_guns
                    || !lex.parse(turret.position[ii])) {
                    break;
                }
                // Trailing comma is allowed
                if (!lex.check_token(",")) {
                    break;
                }
            }
            if (!lex.has_error()) {
                lex.expect_token("]");
                lex.expect_token(";");
            }
        } else if (check_field(lex, "reload_time", turret.reload_time, units_time)) {
        } else if (lex.check_token("gun_design")) {
            lexer::token design;
            if (lex.expect_token("=")
                && lex.expect_token_type(design, lexer::token_type::name)
                && lex.expect_token(";")) {
                if (!(turret.gun_design = mgr.find_gun(design))) {
                    lex.set_error(design, "failed to find gun design '%.*s'",
                        int(design.end - design.begin), design.begin);
                }
            } else {
                return false;
            }
        } else if (check_field(lex, "outline", turret.outline)) {
        } else {
            lexer::token t;
            if (lex.expect_any_token(t)) {
                lex.set_error(t, "unrecognized field '%.*s'", int(t.end - t.begin), t.begin);
            }
            return false;
        }
    }

    // Create barbette outline
    if (turret.radius) {
        vec2 v[64];
        for (std::size_t ii = 0; ii < countof(v) / 4; ++ii) {
            float a = float(ii) * (math::twopi / float(countof(v)));
            // render::outline expects vertices in clockwise order starting with quadrant II
            v[ii] = vec2(-cos(a) * turret.radius, sin(a) * turret.radius);
            v[ii + (countof(v) * 1 / 4)] = vec2(v[ii].y, -v[ii].x);
            v[ii + (countof(v) * 2 / 4)] = vec2(-v[ii].x, -v[ii].y);
            v[ii + (countof(v) * 3 / 4)] = vec2(-v[ii].y, v[ii].x);
        }
        turret.barbette_outline = render::outline(v, countof(v));
    }

    if (!lex.has_error()) {
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
void turret_design::print(file::stream& s, turret_design const& turret)
{
    s.printf("%s = {\n", turret.id.c_str());
    s.printf("    radius = %g m;\n", turret.radius);
    s.printf("    train_speed = %g;\n", math::rad2deg(turret.train_speed));
    s.printf("    elevation_speed = %g;\n", math::rad2deg(turret.elevation_speed));
    s.printf("    elevation_limit = (%lg, %lg);\n",
        math::rad2deg(turret.elevation_limit.x),
        math::rad2deg(turret.elevation_limit.y));
    s.printf("\n");

    s.printf("    num_guns = %d;\n", turret.num_guns);
    s.printf("    position = [\n");
    for (int ii = 0; ii < turret.num_guns; ++ii) {
        s.printf("        (%lg, %lg, %lg),\n",
            turret.position[ii].x,
            turret.position[ii].y,
            turret.position[ii].z);
    }
    s.printf("    ];\n");
    s.printf("    reload_time = %lg s;\n", turret.reload_time.to_seconds());
    s.printf("\n");

    s.printf("    gun_design = %s;\n", turret.gun_design->id.c_str());
    s.printf("\n");

    s.printf("    outline = [\n");
    for (std::size_t ii = 0; ii < turret.outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", turret.outline[ii].x, turret.outline[ii].y);
    }
    s.printf("    ];\n");

    s.printf("};\n");
    s.close();
}

} // namespace game
