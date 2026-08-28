// g_ship_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_ship_design.h"
#include "design/g_design_parser.h"
#include "design/g_funnel_design.h"
#include "design/g_turret_design.h"
#include "cm_filesystem.h"
#include "cm_lexer.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
bool ship_design::parse(lexer& lex, design_manager const& mgr, ship_design& ship)
{
    if (!lex.expect_token("{")) {
        return false;
    }

    while (!lex.has_error() && !lex.check_token("}")) {
        if (check_field(lex, "name", ship.name)) {
        } else if (check_field(lex, "length", ship.length, units_length)) {
        } else if (check_field(lex, "beam", ship.beam, units_length)) {
        } else if (check_field(lex, "draft", ship.draft, units_length)) {
        } else if (check_field(lex, "displacement", ship.displacement, units_mass)) {
        } else if (check_field(lex, "speed", ship.speed, units_speed)) {
        } else if (check_field(lex, "power", ship.power, units_power)) {
        } else if (check_field(lex, "rudder_angle", ship.rudder_angle, units_angle)) {
        } else if (check_field(lex, "rudder_speed", ship.rudder_speed, units_angle)) {
        } else if (check_field(lex, "minimum_turning_radius", ship.minimum_turning_radius, units_length)) {
        } else if (check_field(lex, "optimal_turning_radius", ship.optimal_turning_radius, units_length)) {
        } else if (lex.check_token("turrets")
            && lex.expect_token("=")
            && lex.expect_token("[")) {
            while (!lex.has_error() && lex.check_token("{")) {
                ship_design::turret_instance ti{};
                while (!lex.has_error() && !lex.check_token("}")) {
                    if (check_field(lex, "position", ti.position)) {
                    } else if (check_field(lex, "orientation", ti.orientation, units_angle)) {
                    } else if (check_field(lex, "train_limit", ti.train_limit, units_angle)) {
                    } else if (lex.check_token("design")) {
                        lexer::token design;
                        if (lex.expect_token("=")
                            && lex.expect_token_type(design, lexer::token_type::name)
                            && lex.expect_token(";")) {
                            if (!(ti.design = mgr.find_turret(design))) {
                                lex.set_error(design, "failed to find turret design '%.*s'",
                                    int(design.end - design.begin), design.begin);
                            }
                        } else {
                            return false;
                        }
                    } else {
                        lexer::token t;
                        if (lex.expect_any_token(t)) {
                            lex.set_error(t, "unrecognized field '%.*s'", int(t.end - t.begin), t.begin);
                        }
                        return false;
                    }
                }
                ship.turrets.push_back(ti);
                // Trailing comma is allowed
                if (!lex.check_token(",")) {
                    break;
                }
            }
            if (!lex.has_error()) {
                lex.expect_token("]");
                lex.expect_token(";");
            }
        } else if (lex.check_token("funnels")
            && lex.expect_token("=")
            && lex.expect_token("[")) {
            while (!lex.has_error() && lex.check_token("{")) {
                ship_design::funnel_instance fi{};
                while (!lex.has_error() && !lex.check_token("}")) {
                    if (lex.check_token("design")) {
                        lexer::token design;
                        if (lex.expect_token("=")
                            && lex.expect_token_type(design, lexer::token_type::name)
                            && lex.expect_token(";")) {
                            if (!(fi.design = mgr.find_funnel(design))) {
                                lex.set_error(design, "failed to find funnel design '%.*s'",
                                    int(design.end - design.begin), design.begin);
                            }
                        } else {
                            return false;
                        }
                    } else if (check_field(lex, "position", fi.position)) {
                    } else {
                        lexer::token t;
                        if (lex.expect_any_token(t)) {
                            lex.set_error(t, "unrecognized field '%.*s'", int(t.end - t.begin), t.begin);
                        }
                        return false;
                    }
                }
                ship.funnels.push_back(fi);
                // Trailing comma is allowed
                if (!lex.check_token(",")) {
                    break;
                }
            }
            if (!lex.has_error()) {
                lex.expect_token("]");
                lex.expect_token(";");
            }
        } else if (lex.check_token("hull_outline")) {
            if (!lex.expect_token("=") || !lex.expect_token("[")) {
                return false;
            }
            vec2 v;
            while (!lex.has_error() && !lex.peek_token("]") && lex.parse(v)) {
                ship.hull_outline.push_back(v);
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
        ship.hull_shape = {{{std::make_unique<physics::convex_shape>(ship.hull_outline.data(), ship.hull_outline.size())}}};
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
void ship_design::print(file::stream& s, ship_design const& ship)
{
    s.printf("%s = {\n", ship.id.c_str());
    s.printf("    name = %s;\n", make_literal(ship.name).c_str());
    s.printf("    length = %g m;\n", ship.length);
    s.printf("    beam = %g m;\n", ship.beam);
    s.printf("    draft = %g m;\n", ship.draft);
    s.printf("    displacement = %g t;\n", ship.displacement * 1e-3f);
    s.printf("\n");

    s.printf("    power = %g shp;\n", ship.power * (1.f / 0.7456999f));
    s.printf("    speed = %g kn;\n", ship.speed * (1.f / 0.5144447f));
    s.printf("\n");

    s.printf("    rudder_angle = %g;\n", math::rad2deg(ship.rudder_angle));
    s.printf("    rudder_speed = %g;\n", math::rad2deg(ship.rudder_speed));
    s.printf("\n");

    s.printf("    minimum_turning_radius = %g m;\n", ship.minimum_turning_radius);
    s.printf("    optimal_turning_radius = %g m;\n", ship.optimal_turning_radius);
    s.printf("\n");

    s.printf("    turrets = [\n");
    for (std::size_t ii = 0; ii < ship.turrets.size(); ++ii) {
        s.printf("        {\n");
        s.printf("            position = (%lg, %lg, %lg);\n",
            ship.turrets[ii].position.x,
            ship.turrets[ii].position.y,
            ship.turrets[ii].position.z);
        s.printf("            orientation = %g;\n",
            math::rad2deg(ship.turrets[ii].orientation));
        s.printf("            train_limit = (%lg, %lg);\n",
            math::rad2deg(ship.turrets[ii].train_limit.x),
            math::rad2deg(ship.turrets[ii].train_limit.y));
        s.printf("\n");

        s.printf("            design = %s;\n", ship.turrets[ii].design->id.c_str());
        s.printf("        },\n");
    }
    s.printf("    ];\n");
    s.printf("\n");

    s.printf("    funnels = [\n");
    for (std::size_t ii = 0; ii < ship.funnels.size(); ++ii) {
        s.printf("        {\n");
        s.printf("            design = %s;\n", ship.funnels[ii].design->id.c_str());
        s.printf("            position = (%lg, %lg, %lg);\n",
            ship.funnels[ii].position.x,
            ship.funnels[ii].position.y,
            ship.funnels[ii].position.z);
        s.printf("        },\n");
    }
    s.printf("    ];\n");
    s.printf("\n");

    s.printf("    hull_outline = [\n");
    for (std::size_t ii = 0; ii < ship.hull_outline.size(); ++ii) {
        s.printf("        (%lg, %lg),\n", ship.hull_outline[ii].x, ship.hull_outline[ii].y);
    }
    s.printf("    ];\n");

    s.printf("};\n");
    s.close();
}

} // namespace game
