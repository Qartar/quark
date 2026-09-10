// g_gun_design.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "design/g_gun_design.h"
#include "design/g_design_parser.h"
#include "cm_filesystem.h"
#include "g_ballistics.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------------
std::vector<gun_design::range_data> populate_table(gun_design const& gun)
{
    std::vector<gun_design::range_data> table;

    static constexpr double min_elevation = math::deg2rad(-2.0);
    static constexpr double max_elevation = math::deg2rad(50.0);

    static constexpr std::size_t table_size = 128;

    table.resize(table_size);
    for (std::size_t ii = 0; ii < table_size; ++ii) {
        double elevation = ii * ((max_elevation - min_elevation) * (1.0 / double(table_size))) + min_elevation;

        vec3 start = globe::lonlat_to_surface(vec2(0,0)) + vec3(5,0,0);
        vec3 pos = start;
        vec3 vel = vec3(sin(elevation), cos(elevation), 0) * gun.shell_velocity;

        table[ii].elevation = elevation;
        table[ii].time = ballistics::simulate(
            pos,
            vel,
            ballistics::curve::G1,
            gun.shell_coefficient,
            FRAMETIME);
        table[ii].range = length(pos - start);
        table[ii].impact = vec2(vel.y, vel.x);

        if (ii > 0 && table[ii].range < table[ii - 1].range) {
            table.resize(ii);
            break;
        }
    }

    return table;
}

} // anonymous namespace

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
        } else if (lex.check_token("range_table") && lex.expect_token("=") && lex.expect_token("[")) {
            range_data d;
            double time;
            while (!lex.has_error() && !lex.peek_token("]")) {
                if (!lex.expect_token("(")
                    || !lex.parse(d.range)
                    || !lex.expect_token(",")
                    || !lex.parse(d.elevation)
                    || !lex.expect_token(",")
                    || !lex.parse(d.impact)
                    || !lex.expect_token(",")
                    || !lex.parse(time)
                    || !lex.expect_token(")")) {
                    break;
                }
                d.elevation = math::deg2rad(d.elevation);
                d.time = time_delta::from_seconds(time);
                gun.range_table.push_back(d);
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

    // Populate range table if not provided explicitly
    if (!gun.range_table.size()) {
        gun.range_table = populate_table(gun);
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

    s.printf("    range_table = [\n");
    for (std::size_t ii = 0; ii < gun.range_table.size(); ++ii) {
        s.printf("        (%lg, %lg, (%lg, %lg), %lg),\n",
            gun.range_table[ii].range,
            math::rad2deg(gun.range_table[ii].elevation),
            gun.range_table[ii].impact.x,
            gun.range_table[ii].impact.y,
            gun.range_table[ii].time.to_seconds());
    }
    s.printf("    ];\n");

    s.printf("};\n");
}

//------------------------------------------------------------------------------
bool gun_design::interpolate_range(double d, range_data& r) const
{
    std::size_t ii = 0, jj = range_table.size() - 1;
    // Check boundary conditions, return best possible solution
    if (d < range_table[ii].range) {
        r = range_table[ii];
        return false;
    } else if (d > range_table[jj].range) {
        r = range_table[jj];
        return false;
    }

    do {
        std::size_t mid = (ii + jj) / 2;
        if (d >= range_table[mid].range) {
            ii = mid;
        } else {
            jj = mid;
        }
    } while (ii + 1 < jj);

    // linear interpolation
    double t = (d - range_table[ii].range) / (range_table[jj].range - range_table[ii].range);
    r.range = d;
    r.elevation = range_table[ii].elevation + (range_table[jj].elevation - range_table[ii].elevation) * t;
    r.impact = range_table[ii].impact + (range_table[jj].impact - range_table[ii].impact) * t;
    r.time = range_table[ii].time + (range_table[jj].time - range_table[ii].time) * t;
    return true;
}

} // namespace game
