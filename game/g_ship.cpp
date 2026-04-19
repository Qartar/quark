// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_faction.h"
#include "g_fire_director.h"
#include "g_navigation.h"
#include "g_projectile.h"
#include "g_subsystem.h"
#include "r_model.h"
#include "design/g_ship_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type ship::_type(object::_type);
physics::material ship::_material(0.5f, 1.0f, 5.0f);

ship_design const* ship_designs[] = {
    &ship_yamato_battleship,
    &ship_north_carolina_battleship,
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

    _engines = get_world()->spawn<game::engines>(this);
    _subsystems.push_back(_engines);

    _navigation = get_world()->spawn<game::navigation>(this);
    _subsystems.push_back(_navigation);

    // add a fire director for each unique gun design
    {
        gun_design const* gun = nullptr;
        for (std::size_t ii = 0; ii < _turrets.size(); ++ii) {
            // assume all similar gun turrets are contiguous in ship design
            if (_design->turrets[ii].design->gun_design != gun) {
                gun = _design->turrets[ii].design->gun_design;
                _fire_directors.push_back(get_world()->spawn<game::fire_director>(this, gun));
                _subsystems.push_back(_fire_directors.back());
            }
            _turrets[ii].fire_director = _fire_directors.back();
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
        update_firing_solution(ii);

        float traverse_target = clamp(
            _turrets[ii].traverse_target,
            _design->turrets[ii].train_limit[0],
            _design->turrets[ii].train_limit[1]);
        float traverse_delta = traverse_target - _turrets[ii].traverse;
        float traverse_max = _design->turrets[ii].design->train_speed * FRAMETIME.to_seconds();
        if (abs(traverse_delta) > traverse_max) {
            _turrets[ii].traverse += std::copysign(traverse_max, traverse_delta);
        } else {
            _turrets[ii].traverse = traverse_target;
        }

        float elevation_target = clamp(
            _turrets[ii].elevation_target,
            _design->turrets[ii].design->elevation_limit[0],
            _design->turrets[ii].design->elevation_limit[1]);
        float elevation_delta = elevation_target - _turrets[ii].elevation;
        float elevation_max = _design->turrets[ii].design->elevation_speed * FRAMETIME.to_seconds();
        if (abs(elevation_delta) > elevation_max) {
            _turrets[ii].elevation += std::copysign(elevation_max, elevation_delta);
        } else {
            _turrets[ii].elevation = elevation_target;
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

        if (!_turrets[idx].fire_director || !_turrets[idx].fire_director->has_solution()) {
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
            /* ballistic_coefficient */ turret.design->gun_design->shell_coefficient,
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
        for (std::size_t ii = 0; ii < _fire_directors.size(); ++ii) {
            _fire_directors[ii]->set_target(_primary_target);
        }
    }
}

//------------------------------------------------------------------------------
void ship::update_firing_solution(std::size_t turret_index)
{
    float bearing, elevation;

    if (_turrets[turret_index].fire_director) {
        _turrets[turret_index].fire_director->get_solution(bearing, elevation);
        // Get bearing relative to turret orientation, normalize to [-pi,pi)
        bearing -= _design->turrets[turret_index].orientation;
        bearing -= math::twopi * std::round(bearing / math::twopi);
        // TODO: parallax corrections
        _turrets[turret_index].traverse_target = bearing;
        _turrets[turret_index].elevation_target = elevation;
    }
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

} // namespace game
