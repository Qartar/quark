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

#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type ship::_type(object::_type);
physics::material ship::_material(0.5f, 1.0f, 5.0f);

ship_design const* ship_designs[] = {
    &ship_yamato_battleship,
    &ship_iowa_battleship,
    &ship_king_george_v_battleship,
    &ship_richelieu_battleship,
    &ship_bismarck_battleship,
    &ship_littorio_battleship,
};

static int ships_idx = 0;

//------------------------------------------------------------------------------
ship::ship(handle<game::faction> faction)
    : _usercmd{}
    , _design(ship_designs[ships_idx++ % countof(ship_designs)])
    , _faction(faction)
    , _wake_index(0)
{
    _rigid_body = physics::rigid_body(&_design->hull_shape, &_material, 1.f);

    _turrets.resize(_design->turrets.size(), {});

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

    _engines = get_world()->spawn<game::engines>(this);
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
        vec2 v0 = _design->hull_outline[0] * tx;
        for (std::size_t ii = 1; ii < _design->hull_outline.size(); ++ii) {
            vec2 v1 = _design->hull_outline[ii] * tx;
            renderer->draw_line(v0, v1, color, color);
            v0 = v1;
        }
        vec2 v1 = _design->hull_outline[0] * tx;
        renderer->draw_line(v0, v1, color, color);
    }

    // draw rudder
    {
        vec2 v0 = vec2(_design->length * -.45f, 0) * tx;
        vec2 vx = vec2(_design->length,0) * get_rotation(time) * rot2(_engines->get_rudder_angle());
        vec2 v1 = v0 - vx * .025f;
        vec2 v2 = v0 + vx * .025f;
        renderer->draw_line(v1, v2, color, color);
    }

    // draw turrets
    for (std::size_t jj = 0, num = _turrets.size(); jj < num; ++jj) {
        auto const& turret = _design->turrets[jj];
        mat3 turret_tx = mat3::transform(turret.position, rot2(turret.orientation + _turrets[jj].traverse)) * tx;

        float radius = turret.design->radius;

        // draw turret outline
        {
            vec2 v0 = turret.design->outline.front() * turret_tx;
            for (std::size_t kk = 1, sz = turret.design->outline.size(); kk < sz; ++kk) {
                vec2 v1 = turret.design->outline[kk] * turret_tx;
                renderer->draw_line(v0, v1, color, color);
                v0 = v1;
            }
            renderer->draw_line(v0, turret.design->outline.front() * turret_tx, color, color);
        }

        float l = 0.7f * cos(_turrets[jj].elevation) * turret.design->gun_design->length;

        // draw guns
        for (int ii = 0; ii < turret.design->num_guns; ++ii) {
            float x = radius;
            float y = turret.design->spacing * (ii - .5f * (turret.design->num_guns - 1));
            vec2 v1 = vec2(x, y);

            float caliber = turret.design->gun_design->caliber;
            vec2 pts[4] = {
                (v1 + vec2(0, 1.25f * caliber)) * turret_tx,
                (v1 + vec2(l, .5f * caliber)) * turret_tx,
                (v1 + vec2(l, -.5f * caliber)) * turret_tx,
                (v1 + vec2(0, -1.25f * caliber)) * turret_tx
            };
            renderer->draw_line(pts[0], pts[1], color, color);
            renderer->draw_line(pts[1], pts[2], color, color);
            renderer->draw_line(pts[2], pts[3], color, color);
        }
    }

    // draw wake
    for (std::size_t ii = 0; ii + 1 < _wake_index && ii + 1 < countof(_wake); ++ii) {
        float a0 = float(countof(_wake) - ii) / float(countof(_wake));
        float a1 = float(countof(_wake) - ii - 1) / float(countof(_wake));
        renderer->draw_line(
            _wake[(_wake_index - ii) % countof(_wake)],
            _wake[(_wake_index - ii - 1) % countof(_wake)],
            color4(.8f,.9f,1.f,.5f * a0),
            color4(.8f,.9f,1.f,.5f * a1));

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

    {
        _wake_index = std::size_t(time.to_seconds() / 1.f);
        _wake[_wake_index % countof(_wake)] = get_position() - vec2(.5f * _design->length, 0) * get_rotation();
    }

    for (std::size_t ii = 0, num = _turrets.size(); ii < num; ++ii) {
        update_firing_solution(_primary_target, ii);

        float traverse_delta = _turrets[ii].traverse_target - _turrets[ii].traverse;
        float traverse_max = _design->turrets[ii].design->train_speed * FRAMETIME.to_seconds();
        if (abs(traverse_delta) > traverse_max) {
            _turrets[ii].traverse += std::copysign(traverse_max, traverse_delta);
        } else {
            _turrets[ii].traverse = _turrets[ii].traverse_target;
        }

        float elevation_delta = _turrets[ii].elevation_target - _turrets[ii].elevation;
        float elevation_max = _design->turrets[ii].design->elevation_speed * FRAMETIME.to_seconds();
        if (abs(elevation_delta) > elevation_max) {
            _turrets[ii].elevation += std::copysign(elevation_max, elevation_delta);
        } else {
            _turrets[ii].elevation = _turrets[ii].elevation_target;
        }
    }

    if (!_primary_target || _random.uniform_real() < .001f) {
        update_targets();
    }

    bool is_firing = (_random.uniform_real() < .01f);

    for (std::size_t idx = 0; idx < _turrets.size(); ++idx) {
        auto const& turret = _design->turrets[idx];

        // Emit smoke particles after firing
        constexpr time_delta smoke_delta = time_delta::from_seconds(1.f);
        if (_turrets[idx].refire_time - time > turret.design->reload_time - smoke_delta) {
            float t = 1.f - (turret.design->reload_time - (_turrets[idx].refire_time - time)) / smoke_delta;

            for (std::size_t ii = 0; ii < _design->turrets[idx].design->num_guns; ++ii) {
                vec3 position, direction, velocity;
                get_firing_vectors(idx, ii, position, direction, velocity);
                get_world()->add_effect(time, effect_type::smoke, position.to_vec2(), direction.to_vec2() * 2.f * t, 3.f * square(t), velocity.to_vec2());
            }
        }

        if (!is_firing || time < _turrets[idx].refire_time) {
            continue;
        }

        if (_turrets[idx].traverse != _turrets[idx].traverse_target) {
            continue;
        }

        if (_turrets[idx].elevation != _turrets[idx].elevation_target) {
            continue;
        }

        projectile_info pinfo = {
            /* damage */            1.5e-4f * turret.design->gun_design->shell_mass,
            /* speed */             turret.design->gun_design->shell_velocity,
            /* diameter */          turret.design->gun_design->caliber,
            /* launch_effect */     effect_type::cannon,
            /* launch_sound */      sound::asset::invalid,
            /* flight_effect */     effect_type::none,
            /* flight sound */      sound::asset::invalid,
            /* impact_effect */     effect_type::cannon_impact,
            /* impact_sound */      sound::asset::invalid,
        };

        for (std::size_t ii = 0; ii < _design->turrets[idx].design->num_guns; ++ii) {
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
                float strength = 1.9e-9f * turret.design->gun_design->shell_mass * square(turret.design->gun_design->shell_velocity);
                get_world()->add_effect(time, pinfo.launch_effect, position.to_vec2(), direction.to_vec2() * 2, strength, velocity.to_vec2());
            }
        }

        _turrets[idx].refire_time = time + turret.design->reload_time;
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
    auto const& turret = _design->turrets[turret_index];
    vec2 dir = normalize(target_pos - turret.position * tx);

    float angle = atan2f(dir.y, dir.x) - get_rotation().radians() - _design->turrets[turret_index].orientation;
    angle -= math::twopi * std::round(angle / math::twopi); // normalize to [-pi,pi)
    angle = clamp(angle, _design->turrets[turret_index].train_limit[0], _design->turrets[turret_index].train_limit[1]);

    _turrets[turret_index].traverse_target = angle;

    // TODO: elevation target

    _turrets[turret_index].elevation_target = _primary_gunnery_table.interpolate(
        length(target_pos - turret.position * tx));
    _turrets[turret_index].elevation_target = clamp(_turrets[turret_index].elevation_target,
                                                    _design->turrets[turret_index].design->elevation_limit[0],
                                                    _design->turrets[turret_index].design->elevation_limit[1]);
}

//------------------------------------------------------------------------------
void ship::get_firing_vectors(std::size_t turret_index, std::size_t gun_index, vec3& position, vec3& direction, vec3& velocity) const
{
    auto const& turret = _design->turrets[turret_index];

    mat3 turret_tx = mat3::transform(
        turret.position,
        rot2(turret.orientation + _turrets[turret_index].traverse)) * get_transform();

    position = vec3(vec2(turret.design->radius + cos(_turrets[turret_index].elevation) * 0.9f * turret.design->gun_design->length,
                         turret.design->spacing * (gun_index - .5f * (turret.design->num_guns - 1))) * turret_tx,
                    8.f + sin(_turrets[turret_index].elevation) * .9f * turret.design->gun_design->length);

    direction = vec3(cos(_turrets[turret_index].elevation) * turret_tx[0][0],
                     cos(_turrets[turret_index].elevation) * turret_tx[0][1],
                     sin(_turrets[turret_index].elevation));

    velocity = vec3(get_linear_velocity() + (position.to_vec2() - get_position()).cross(get_angular_velocity()), 0);
}

//------------------------------------------------------------------------------
void ship::populate_gunnery_tables()
{
    float range[256];

    vec2 elevation = _design->turrets[0].design->elevation_limit;

    for (std::size_t ii = 0; ii < countof(range); ++ii) {
        float x = elevation[0] + (elevation[1] - elevation[0]) * ii / (countof(range) - 1);

        vec3 pos = vec3(0, 0, 8.f); // TODO: turret height
        vec3 vel = vec3(cos(x), 0, sin(x)) * _design->turrets[0].design->gun_design->shell_velocity;

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
