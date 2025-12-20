// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_character.h"
#include "g_faction.h"
#include "g_navigation.h"
#include "g_shield.h"
#include "g_weapon.h"
#include "g_subsystem.h"
#include "r_model.h"
#include "cm_ballistics.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type ship::_type(object::_type);
physics::material ship::_material(0.5f, 1.0f, 5.0f);

#define QBZ(a,b,c,t)        \
    (((1-t)*(1-t)*(a)+2*(1-t)*t*(b)+t*t*(c)))

#define QBZ8(a,b,c)         \
    QBZ(a,b,c,0),           \
    QBZ(a,b,c,0.125f),      \
    QBZ(a,b,c,0.25f),       \
    QBZ(a,b,c,0.375f),      \
    QBZ(a,b,c,0.5f),        \
    QBZ(a,b,c,0.625f),      \
    QBZ(a,b,c,0.75f),       \
    QBZ(a,b,c,0.875)

#define SHIP(L,B) {         \
    QBZ8(vec2(0.5f * L, 0.f), vec2(0.3f * L, 0.5f * B), vec2(0.f, 0.5f * B)),   \
    QBZ8(vec2(0.f, 0.5f * B), vec2(-0.5f * L, 0.5f * B), vec2(-0.5f * L, 0.f)),   \
    QBZ8(vec2(-0.5f * L, 0.f), vec2(-0.5f * L, -0.5f * B), vec2(0.f, -0.5f * B)),   \
    QBZ8(vec2(0.f, -0.5f * B), vec2(0.3f * L, -0.5f * B), vec2(0.5f * L, 0.f)),   }

const vec2 ship_hulls[][32] = {
    // yamato-class battleship
    SHIP(263.f, 39.f),

    // iowa-class battleship
    SHIP(270.f, 33.f),

    // king george v-class battleship
    SHIP(227.f, 31.5f),

    // deutschland-class cruiser
    SHIP(186.f, 21.7f),

    // town-class cruiser
    SHIP(180.f, 19.f),

    // tribal-class destroyer
    SHIP(115.f, 11.f),
};

const vec2 yamato_verts[] = {
    vec2(131.50f, 0.00f), vec2(126.49f, 6.20f), vec2(52.90f, 17.40f), vec2(23.41f, 19.42f), vec2(-73.95f, 19.60f), vec2(-77.85f, 17.52f), vec2(-99.05f, 17.90f), vec2(-101.14f, 16.36f), vec2(-101.15f, 13.87f), vec2(-128.36f, 4.63f), vec2(-131.50f, 0.00f), vec2(-128.36f, -4.63f), vec2(-101.15f, -13.87f), vec2(-101.14f, -16.36f), vec2(-99.05f, -17.90f), vec2(-77.85f, -17.52f), vec2(-73.95f, -19.60f), vec2(23.41f, -19.42f), vec2(52.90f, -17.40f), vec2(126.49f, -6.20f)
};

const ship_info ships_info[] =
{
    {
        string::buffer("Yamato-class battleship"),
        263.f, 39.f, 0.f,
        //{{{std::make_unique<physics::convex_shape>(SHIP(263.f, 39.f))}}},
        {{{std::make_unique<physics::convex_shape>(yamato_verts)}}},
        std::vector<vec2>(yamato_verts, yamato_verts + countof(yamato_verts)),
        {
            // 46cm/45 Type 94
            {vec2(52,0), 7.f, 0, .75f * vec2(-math::pi, math::pi), .04f, math::deg2rad(vec2(-5, 45)), math::deg2rad(10.f), time_delta::from_seconds(24.f), 3, 2.9f, .46f, 20.7f, 1460, 780},
            {vec2(30,0), 7.f, 0, .75f * vec2(-math::pi, math::pi), .04f, math::deg2rad(vec2(-5, 45)), math::deg2rad(10.f), time_delta::from_seconds(24.f), 3, 2.9f, .46f, 20.7f, 1460, 780},
            {vec2(-65,0), 7.f, math::pi, .75f * vec2(-math::pi, math::pi), .04f, math::deg2rad(vec2(-5, 45)), math::deg2rad(10.f), time_delta::from_seconds(24.f), 3, 2.9f, .46f, 20.7f, 1460, 780},
        }
    },
    {
        string::buffer("Iowa-class battleship"),
        270.f, 33.f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[1])}}},
        SHIP(270.f, 33.f),
        {
            // 16"/50 Mark 7
            {vec2(48,0), 6.5f, 0, .75f * vec2(-math::pi, math::pi), .07f, math::deg2rad(vec2(-5, 45)), math::deg2rad(12.f), time_delta::from_seconds(30.f), 3, 2.25f, .406f, 20.f, 1225, 762},
            {vec2(24,0), 6.5f, 0, .75f * vec2(-math::pi, math::pi), .07f, math::deg2rad(vec2(-5, 45)), math::deg2rad(12.f), time_delta::from_seconds(30.f), 3, 2.25f, .406f, 20.f, 1225, 762},
            {vec2(-48,0), 6.5f, math::pi, .75f * vec2(-math::pi, math::pi), .07f, math::deg2rad(vec2(-5, 45)), math::deg2rad(12.f), time_delta::from_seconds(30.f), 3, 2.25f, .406f, 20.f, 1225, 762},
        }
    },
    {
        string::buffer("King George V-class battleship"),
        227.f, 31.5f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[2])}}},
        SHIP(227.f, 31.5f),
        {
            // BL 14-inch Mark VII
            {vec2(40,0), 6.f, 0, .75f * vec2(-math::pi, math::pi), .07f, math::deg2rad(vec2(-5, 41)), math::deg2rad(10.f), time_delta::from_seconds(30.f), 4, 2.f, .3556f, 16.f, 724, 757},
            {vec2(16,0), 6.f, 0, .75f * vec2(-math::pi, math::pi), .07f, math::deg2rad(vec2(-5, 41)), math::deg2rad(10.f), time_delta::from_seconds(30.f), 2, 2.f, .3556f, 16.f, 724, 757},
            {vec2(-40,0), 6.f, math::pi, .75f * vec2(-math::pi, math::pi), .07f, math::deg2rad(vec2(-5, 41)), math::deg2rad(10.f), time_delta::from_seconds(30.f), 4, 2.f, .3556f, 16.f, 724, 757},
        }
    },
    {
        string::buffer("Deutschland-class heavy cruiser"),
        186.f, 21.7f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[3])}}},
        SHIP(186.f, 21.7f),
        {
            // 28 cm SK C/28
            {vec2(32,0), 5.5f, 0, .75f * vec2(-math::pi, math::pi), .09f, math::deg2rad(vec2(-10, 40)), math::deg2rad(15.f), time_delta::from_seconds(24.f), 3, 1.75f, .28f, 13.9f, 300, 910},
            {vec2(-32,0), 5.5f, math::pi, .75f * vec2(-math::pi, math::pi), .09f, math::deg2rad(vec2(-10, 40)), math::deg2rad(15.f), time_delta::from_seconds(24.f), 3, 1.75f, .28f, 13.9f, 300, 910},
        }
    },
    {
        string::buffer("Town-class light cruiser"),
        180.f, 19.f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[4])}}},
        SHIP(180.f, 19.f),
        {
            // BL 6-inch Mark XXIII
            {vec2(32,0), 3.f, 0, .75f * vec2(-math::pi, math::pi), .12f, math::deg2rad(vec2(-5, 45)), math::deg2rad(20.f), time_delta::from_seconds(8.f), 3, 1.f, .152f, 7.6f, 51, 840},
            {vec2(16,0), 3.f, 0, .75f * vec2(-math::pi, math::pi), .12f, math::deg2rad(vec2(-5, 45)), math::deg2rad(20.f), time_delta::from_seconds(8.f), 3, 1.f, .152f, 7.6f, 51, 840},
            {vec2(-16,0), 3.f, math::pi, .75f * vec2(-math::pi, math::pi), .12f, math::deg2rad(vec2(-5, 45)), math::deg2rad(20.f), time_delta::from_seconds(8.f), 3, 1.f, .152f, 7.6f, 51, 840},
            {vec2(-32,0), 3.f, math::pi, .75f * vec2(-math::pi, math::pi), .12f, math::deg2rad(vec2(-5, 45)), math::deg2rad(20.f), time_delta::from_seconds(8.f), 3, 1.f, .152f, 7.6f, 51, 840},
        }
    },
    {
        string::buffer("Tribal-class destroyer"),
        115.f, 11.f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[5])}}},
        SHIP(115.f, 11.f),
        {
            // QF 4.7-inch Mark IX & XII
            {vec2(24,0), 2.f, 0, .75f * vec2(-math::pi, math::pi), .15f, math::deg2rad(vec2(-10, 40)), math::deg2rad(25.f), time_delta::from_seconds(4.f), 2, 0.75f, .12f, 5.4f, 23, 810},
            {vec2(12,0), 2.f, 0, .75f * vec2(-math::pi, math::pi), .15f, math::deg2rad(vec2(-10, 40)), math::deg2rad(25.f), time_delta::from_seconds(4.f), 2, 0.75f, .12f, 5.4f, 23, 810},
            {vec2(-12,0), 2.f, math::pi, .75f * vec2(-math::pi, math::pi), .15f, math::deg2rad(vec2(-10, 40)), math::deg2rad(25.f), time_delta::from_seconds(4.f), 2, 0.75f, .12f, 5.4f, 23, 810},
            {vec2(-24,0), 2.f, math::pi, .75f * vec2(-math::pi, math::pi), .15f, math::deg2rad(vec2(-10, 40)), math::deg2rad(25.f), time_delta::from_seconds(4.f), 2, 0.75f, .12f, 5.4f, 23, 810},
        }
    },
};

static int ships_idx = 0;

//------------------------------------------------------------------------------
ship::ship(handle<game::faction> faction)
    : _usercmd{}
    , _info(&ships_info[ships_idx++ % countof(ships_info)])
    , _faction(faction)
{
    _rigid_body = physics::rigid_body(&_info->shape, &_material, 1.f);

    _turrets.resize(_info->turrets.size(), {});

    _model = &ship_model;

    populate_gunnery_tables();
}

//------------------------------------------------------------------------------
ship::~ship()
{
    get_world()->remove_body(&_rigid_body);
}

//------------------------------------------------------------------------------
void ship::spawn()
{
    object::spawn();

    get_world()->add_body(this, &_rigid_body);

    for (int ii = 0; ii < 3; ++ii) {
        _crew.push_back(get_world()->spawn<character>());
    }

    _reactor = get_world()->spawn<subsystem>(this, subsystem_info{subsystem_type::reactor, 13});
    _subsystems.push_back(_reactor);

    _engines = get_world()->spawn<game::engines>(this, engines_info{16.f, .05f, 8.f, .0625f, .5f, .5f});
    _subsystems.push_back(_engines);

    _navigation = get_world()->spawn<game::navigation>(this);
    _subsystems.push_back(_navigation);

    std::vector<handle<subsystem>> assignments(_subsystems.begin(), _subsystems.end());
    for (auto& ch : _crew) {
        if (assignments.size()) {
            std::size_t index = _random.uniform_int(assignments.size());
            ch->assign(assignments[index]);
            assignments.erase(assignments.begin() + index);
        }
    }
}

//------------------------------------------------------------------------------
void ship::draw(render::system* renderer, time_value time) const
{
    color4 color = _faction ? _faction->color() : color4(.8f,.9f,1.f,1.f);
    auto tx = get_transform(time);

    // draw hull outline
    {
        vec2 v0 = _info->outline[0] * tx;
        for (std::size_t ii = 1; ii < _info->outline.size(); ++ii) {
            vec2 v1 = _info->outline[ii] * tx;
            renderer->draw_line(v0, v1, color, color);
            v0 = v1;
        }
        vec2 v1 = _info->outline[0] * tx;
        renderer->draw_line(v0, v1, color, color);
    }

    // draw turrets
    for (std::size_t jj = 0, num = _turrets.size(); jj < num; ++jj) {
        auto const& turret = _info->turrets[jj];
        mat3 turret_tx = mat3::transform(turret.position, rot2(turret.orientation + _turrets[jj].traverse)) * tx;

        // draw turret outline
        vec2 f1 = vec2(turret.radius, .6f * turret.radius) * turret_tx;
        vec2 f2 = vec2(turret.radius, -.6f * turret.radius) * turret_tx;
        vec2 m1 = vec2(.3f * turret.radius, turret.radius) * turret_tx;
        vec2 m2 = vec2(.3f * turret.radius, -turret.radius) * turret_tx;
        vec2 m3 = vec2(-.3f * turret.radius, turret.radius) * turret_tx;
        vec2 m4 = vec2(-.3f * turret.radius, -turret.radius) * turret_tx;
        vec2 r1 = vec2(-2.f * turret.radius, .8f * turret.radius) * turret_tx;
        vec2 r2 = vec2(-2.f * turret.radius, -.8f * turret.radius) * turret_tx;

        renderer->draw_line(f1, f2, color, color);
        renderer->draw_line(f1, m1, color, color);
        renderer->draw_line(f2, m2, color, color);
        renderer->draw_line(m1, m3, color, color);
        renderer->draw_line(m2, m4, color, color);
        renderer->draw_line(m3, r1, color, color);
        renderer->draw_line(m4, r2, color, color);
        renderer->draw_line(r1, r2, color, color);

        float l = 0.9f * cos(_turrets[jj].elevation) * turret.length;

        // draw guns
        for (int ii = 0; ii < turret.num_guns; ++ii) {
            float x = turret.radius;
            float y = turret.spacing * (ii - .5f * (turret.num_guns - 1));
            vec2 v1 = vec2(x, y);

            vec2 pts[4] = {
                (v1 + vec2(0, 1.5f * turret.caliber)) * turret_tx,
                (v1 + vec2(l, .5f * turret.caliber)) * turret_tx,
                (v1 + vec2(l, -.5f * turret.caliber)) * turret_tx,
                (v1 + vec2(0, -1.5f * turret.caliber)) * turret_tx
            };
            renderer->draw_line(pts[0], pts[1], color, color);
            renderer->draw_line(pts[1], pts[2], color, color);
            renderer->draw_line(pts[2], pts[3], color, color);
        }
    }
}

//------------------------------------------------------------------------------
bool ship::touch(object* /*other*/, physics::collision const* /*collision*/)
{
    return true;
}

//------------------------------------------------------------------------------
void ship::think()
{
    time_value time = get_world()->frametime();

    for (std::size_t ii = 0, num = _turrets.size(); ii < num; ++ii) {
        update_firing_solution(_primary_target, ii);

        float traverse_delta = _turrets[ii].traverse_target - _turrets[ii].traverse;
        float traverse_max = _info->turrets[ii].traverse_speed * FRAMETIME.to_seconds();
        if (abs(traverse_delta) > traverse_max) {
            _turrets[ii].traverse += std::copysign(traverse_max, traverse_delta);
        } else {
            _turrets[ii].traverse = _turrets[ii].traverse_target;
        }

        float elevation_delta = _turrets[ii].elevation_target - _turrets[ii].elevation;
        float elevation_max = _info->turrets[ii].elevation_speed * FRAMETIME.to_seconds();
        if (abs(elevation_delta) > elevation_max) {
            _turrets[ii].elevation += std::copysign(elevation_max, elevation_delta);
        } else {
            _turrets[ii].elevation = _turrets[ii].elevation_target;
        }
    }

    if (_random.uniform_real() < .01f) {
        update_targets();
    }

    if (_random.uniform_real() < .01f) {
        for (std::size_t idx = 0; idx < _turrets.size(); ++idx) {
            auto const& turret = _info->turrets[idx];

            if (time < _turrets[idx].refire_time) {
                continue;
            }

            if (_turrets[idx].traverse != _turrets[idx].traverse_target) {
                continue;
            }

            if (_turrets[idx].elevation != _turrets[idx].elevation_target) {
                continue;
            }

            projectile_info pinfo = {
                /* damage */            3e-4f * turret.shell_mass,
                /* speed */             turret.shell_velocity,
                /* diameter */          turret.caliber,
                /* launch_effect */     effect_type::cannon,
                /* launch_sound */      sound::asset::invalid,
                /* flight_effect */     effect_type::none,
                /* flight sound */      sound::asset::invalid,
                /* impact_effect */     effect_type::cannon_impact,
                /* impact_sound */      sound::asset::invalid,
            };

            for (std::size_t ii = 0; ii < _info->turrets[idx].num_guns; ++ii) {
                vec3 position, direction, velocity;
                get_firing_vectors(idx, ii, position, direction, velocity);

                // add dispersion
                direction = normalize(direction + vec3(_random.normal_real(1e-3f),
                                                       _random.normal_real(1e-3f),
                                                       _random.normal_real(1e-3f)));

                get_world()->spawn<projectile>(
                    this,
                    pinfo,
                    position,
                    direction * pinfo.speed + velocity);

                if (pinfo.launch_effect != effect_type::none) {
                    get_world()->add_effect(time, pinfo.launch_effect, position.to_vec2(), direction.to_vec2() * 2);
                }
            }

            _turrets[idx].refire_time = time + turret.reload_time;
        }
    }
}

//------------------------------------------------------------------------------
void ship::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void ship::write_snapshot(network::message& /*message*/) const
{
}

//------------------------------------------------------------------------------
void ship::damage(object* /*inflictor*/, vec2 /*point*/, float /*amount*/)
{
}

//------------------------------------------------------------------------------
void ship::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

//------------------------------------------------------------------------------
void ship::update_targets()
{
    std::vector<game::ship const*> targets;
    for (auto const* object : get_world()->objects()) {
        if (object->is_type<ship>()) {
            auto f = object->cast<ship>()->faction();
            if (f != _faction) {
                targets.push_back(object->cast<ship>());
            }
        }
    }

    if (targets.size()) {
        _primary_target = targets[_random.uniform_int(targets.size())];
    }
}

//------------------------------------------------------------------------------
void ship::update_firing_solution(handle<ship const> target, std::size_t turret_index)
{
    vec2 target_pos = target ? target->get_position() : vec2_zero;

    // TODO: target prediction

    auto tx = get_transform();
    auto const& turret = _info->turrets[turret_index];
    vec2 dir = normalize(target_pos - turret.position * tx);

    float angle = atan2f(dir.y, dir.x) - get_rotation().radians() - _info->turrets[turret_index].orientation;
    angle -= math::twopi * std::round(angle / math::twopi); // normalize to [-pi,pi)
    angle = clamp(angle, _info->turrets[turret_index].traverse[0], _info->turrets[turret_index].traverse[1]);

    _turrets[turret_index].traverse_target = angle;

    // TODO: elevation target

    _turrets[turret_index].elevation_target = _primary_gunnery_table.interpolate(
        length(target_pos - turret.position * tx));
}

//------------------------------------------------------------------------------
void ship::get_firing_vectors(std::size_t turret_index, std::size_t gun_index, vec3& position, vec3& direction, vec3& velocity) const
{
    auto const& turret = _info->turrets[turret_index];

    mat3 turret_tx = mat3::transform(
        turret.position,
        rot2(turret.orientation + _turrets[turret_index].traverse)) * get_transform();

    position = vec3(vec2(turret.radius + cos(_turrets[turret_index].elevation) * 0.9f * turret.length,
                         turret.spacing * (gun_index - .5f * (turret.num_guns - 1))) * turret_tx,
                    8.f + sin(_turrets[turret_index].elevation) * .9f * turret.length);

    direction = vec3(cos(_turrets[turret_index].elevation) * turret_tx[0][0],
                     cos(_turrets[turret_index].elevation) * turret_tx[0][1],
                     sin(_turrets[turret_index].elevation));

    velocity = vec3(get_linear_velocity() + (position.to_vec2() - get_position()).cross(get_angular_velocity()), 0);
}

//------------------------------------------------------------------------------
void ship::populate_gunnery_tables()
{
    float range[256];

    vec2 elevation = _info->turrets[0].elevation;

    for (std::size_t ii = 0; ii < countof(range); ++ii) {
        float x = elevation[0] + (elevation[1] - elevation[0]) * ii / (countof(range) - 1);

        vec3 pos = vec3(0, 0, 8.f); // TODO: turret height
        vec3 vel = vec3(cos(x), 0, sin(x)) * _info->turrets[0].shell_velocity;

        //ballistics::simulate(pos, vel, 2e-5f, time_delta::from_seconds(1));
        ballistics::simulate(pos, vel, 2e-6f, FRAMETIME);

        range[ii] = pos.x;
    }

    table<float> range_from_elevation((elevation[1] - elevation[0]) / (countof(range) - 1), elevation[0], range);

    float x[44];

    for (std::size_t ii = 1; ii < countof(x); ++ii) {
        float r = ii * 1000.f;

        float lo = 0.f, hi = 1.f, t = .5f;

        float r0 = r;
        for (std::size_t jj = 0; jj < 32; ++jj) {
            float elev = elevation[0] + (elevation[1] - elevation[0]) * t;
            r0 = range_from_elevation.interpolate(elev);
            if (r0 > r) {
                hi = t;
            } else {
                lo = t;
            }
            t = .5f * (lo + hi);
        }

        x[ii - 1] = elevation[0] + (elevation[1] - elevation[0]) * t;
    }

    _primary_gunnery_table = table(1000.f, 1000.f, x);
}

} // namespace game
