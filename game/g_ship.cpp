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
#include "design/g_gun_design.h"
#include "design/g_ship_design.h"
#include "design/g_turret_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type ship::_type(object::_type);
physics::material ship::_material(0.5f, 1.0f, 5.0f);

//------------------------------------------------------------------------------
ship::ship(ship_design const* design, handle<game::faction> faction)
    : _usercmd{}
    , _design(design)
    , _faction(faction)
    , _wake_index(0)
{
    _rigid_body = physics::rigid_body(&_design->hull_shape, &_material, 1.0);

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

    _outlines.push_back(render::outline(_design->hull_outline.data(), _design->hull_outline.size()));

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

    // add turret and gun outlines
    // TODO: render outlines should be on the designs themselves instead of duplicated on every ship
    {
        for (std::size_t ii = 0; ii < _turrets.size(); ++ii) {
            _turrets[ii].turret_outline = SIZE_MAX;
            _turrets[ii].barbette_outline = SIZE_MAX;
            _turrets[ii].gun_outline = SIZE_MAX;
            // turret designs are not necessarily 1:1 with gun designs, check all preceding designs
            for (std::size_t jj = 0; jj < ii; ++jj) {
                if (_design->turrets[ii].design == _design->turrets[jj].design) {
                    _turrets[ii].turret_outline = _turrets[jj].turret_outline;
                    _turrets[ii].barbette_outline = _turrets[jj].barbette_outline;
                }
                if (_design->turrets[ii].design->gun_design == _design->turrets[jj].design->gun_design) {
                    _turrets[ii].gun_outline = _turrets[jj].gun_outline;
                }
            }

            if (_turrets[ii].turret_outline == SIZE_MAX) {
                _turrets[ii].turret_outline = _outlines.size();
                _outlines.push_back(render::outline(
                    _design->turrets[ii].design->outline.data(),
                    _design->turrets[ii].design->outline.size()));

            }

            if (_turrets[ii].barbette_outline == SIZE_MAX) {
                _turrets[ii].barbette_outline = _outlines.size();
                vec2 outline_verts[64];
                for (std::size_t jj = 0; jj < countof(outline_verts); ++jj) {
                    double a = jj * (math::twopi / double(countof(outline_verts)));
                    outline_verts[jj] = vec2(std::cos(a), std::sin(a)) * _design->turrets[ii].design->radius;
                }
                _outlines.push_back(render::outline(outline_verts, countof(outline_verts)));
            }

            if (_turrets[ii].gun_outline == SIZE_MAX) {
                _turrets[ii].gun_outline = _outlines.size();
                _outlines.push_back(render::outline(
                    _design->turrets[ii].design->gun_design->outline.data(),
                    _design->turrets[ii].design->gun_design->outline.size()));
            }
        }
    }
}

//------------------------------------------------------------------------------
void ship::draw(render::system* renderer, time_value time) const
{
    color4 color = _faction ? _faction->color() : color4(.8f,.9f,1.f,1.f);

    // draw hull outline
    mat4 tx4 = get_transform(time);

    // blend color with sea color to give semi-transparent effect, but render solid
    color4 hull_color = color * .25f + color4(.1f,.2f,.4f,1) * .75f;
    renderer->draw_outline(_outlines[0], tx4, color, hull_color);

    // draw rudder
    {
        vec3 v0 = vec3(_design->length * -0.45, 0, 0) * tx4;
        vec3 vx = vec3(vec2(_design->length,0) * rot2(_engines->get_rudder_angle())) * get_rotation(time);
        vec2 v1 = ((v0 - vx * 0.025) * renderer->view().transform).to_vec2();
        vec2 v2 = ((v0 + vx * 0.025) * renderer->view().transform).to_vec2();
        renderer->draw_line(v1, v2, color, color);
    }

    // draw turrets
    for (std::size_t jj = 0, num = _turrets.size(); jj < num; ++jj) {
        auto const& turret = _design->turrets[jj];
        rot2 turret_rx = rot2(turret.orientation + _turrets[jj].traverse);

        // draw turret outline
        mat4 turret_tx4 = mat4(turret_rx[0], turret_rx[1], 0, 0,
                              -turret_rx[1], turret_rx[0], 0, 0,
                               0,             0,           1, 0,
                               turret.position.x, turret.position.y, turret.position.z, 1) * tx4;

        color4 fill_color = color * .25f + hull_color * .75f;
        renderer->draw_outline(_outlines[_turrets[jj].turret_outline], turret_tx4, color, fill_color);

        // draw barbette outline
        mat4 barbette_tx4 = mat4(1, 0, 0, 0,
                                 0, 1, 0, 0,
                                 0, 0, 1, 0,
                                 turret.position.x, turret.position.y, turret.position.z - 0.1, 1) * tx4;

        color4 barbette_color = color + (hull_color - color) * sqrt(.75f);
        color4 barbette_edge = color * .75f + color4(.1f,.2f,.4f,1) * .25f;
        renderer->draw_outline(_outlines[_turrets[jj].barbette_outline], barbette_tx4, barbette_edge, barbette_color);

        double cp = cos(_turrets[jj].elevation);
        double sp = sin(_turrets[jj].elevation);

        // draw guns
        for (int ii = 0; ii < turret.design->num_guns; ++ii) {
            vec3 p = turret.design->position[ii];

            mat4 gun_tx4 = mat4(cp, 0, sp, 0,
                                0,  1, 0,  0,
                               -sp, 0, cp, 0,
                                p.x, p.y, p.z, 1) * turret_tx4;

            renderer->draw_outline(_outlines[_turrets[jj].gun_outline], gun_tx4, color, fill_color);
        }
    }

    // draw wake
    for (std::size_t ii = 0; ii + 1 < _wake_index && ii + 1 < countof(_wake); ++ii) {
        float a0 = square(float(countof(_wake) - ii) / float(countof(_wake)));
        float a1 = square(float(countof(_wake) - ii - 1) / float(countof(_wake)));
        renderer->draw_line(
            (_wake[(_wake_index - ii) % countof(_wake)] * renderer->view().transform).to_vec2(),
            (_wake[(_wake_index - ii - 1) % countof(_wake)] * renderer->view().transform).to_vec2(),
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
        _wake_index = std::size_t(time.to_seconds() / 2.f);
        _wake[_wake_index % countof(_wake)] = get_position() - vec3(.5f * _design->length, 0, 0) * get_rotation();
    }

    for (std::size_t ii = 0, num = _turrets.size(); ii < num; ++ii) {
        update_firing_solution(ii);

        double traverse_target = clamp(
            _turrets[ii].traverse_target,
            _design->turrets[ii].train_limit[0],
            _design->turrets[ii].train_limit[1]);
        double traverse_delta = traverse_target - _turrets[ii].traverse;
        double traverse_max = _design->turrets[ii].design->train_speed * FRAMETIME.to_seconds();
        if (abs(traverse_delta) > traverse_max) {
            _turrets[ii].traverse += std::copysign(traverse_max, traverse_delta);
        } else {
            _turrets[ii].traverse = traverse_target;
        }

        double elevation_target = clamp(
            _turrets[ii].elevation_target,
            _design->turrets[ii].design->elevation_limit[0],
            _design->turrets[ii].design->elevation_limit[1]);
        double elevation_delta = elevation_target - _turrets[ii].elevation;
        double elevation_max = _design->turrets[ii].design->elevation_speed * FRAMETIME.to_seconds();
        if (abs(elevation_delta) > elevation_max) {
            _turrets[ii].elevation += std::copysign(elevation_max, elevation_delta);
        } else {
            _turrets[ii].elevation = elevation_target;
        }
    }

    if (!_primary_target || _random.uniform_real() < .0001f) {
        update_targets();
    }

    bool is_firing = (_random.uniform_real() < .01f);

    for (std::size_t idx = 0; idx < _turrets.size(); ++idx) {
        auto const& turret = _design->turrets[idx];

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
            direction = normalize(direction + vec3(_random.normal_real(5e-3f),
                                                   _random.normal_real(5e-3f),
                                                   _random.normal_real(5e-3f)));

            get_world()->spawn<projectile>(
                this,
                pinfo,
                position,
                direction * pinfo.speed + velocity);

            if (pinfo.launch_effect != effect_type::none) {
                float strength = 1.9e-9f * turret.design->gun_design->shell_mass * square(turret.design->gun_design->shell_velocity);
                get_world()->add_effect(
                    time,
                    pinfo.launch_effect,
                    position,
                    direction * 2,
                    strength,
                    velocity);
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
void ship::set_heading(rot2 heading, bool teleport)
{
    mat3 proj = globe::surface_projection(get_position()).submatrix<3,3>();
    mat3 tx = mat3(heading.x, heading.y, 0, -heading.y, heading.x, 0, 0, 0, 1) * proj;
    set_rotation(tx.to_rotation(), teleport);
}

//------------------------------------------------------------------------------
void ship::damage(object* /*inflictor*/, vec3 /*point*/, float /*amount*/)
{
}

//------------------------------------------------------------------------------
bool ship::splash_observation(handle<ship const> target, float shell_size, vec3 splash_origin)
{
    for (std::size_t ii = 0; ii < _fire_directors.size(); ++ii) {
        if (_fire_directors[ii]->splash_observation(target, shell_size, splash_origin)) {
            return true;
        }
    }
    return false;
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
    double range, bearing, elevation;

    if (_turrets[turret_index].fire_director && _turrets[turret_index].fire_director->get_target()) {
        _turrets[turret_index].fire_director->get_solution(range, bearing, elevation);
        vec2 parallax = _design->turrets[turret_index].position.to_vec2() * rot2(-bearing);
        // Get bearing relative to turret orientation
        bearing -= _design->turrets[turret_index].orientation;
        // Parallax correction using small angle approximation
        bearing -= parallax.y / range;
        // Normalize to [-pi,pi)
        bearing -= math::twopi * std::round(bearing / math::twopi);
        _turrets[turret_index].traverse_target = bearing;
        _turrets[turret_index].elevation_target = elevation;
    } else {
        _turrets[turret_index].traverse_target = 0;
        _turrets[turret_index].elevation_target = 0;
    }
}

//------------------------------------------------------------------------------
void ship::get_firing_vectors(std::size_t turret_index, std::size_t gun_index, vec3& position, vec3& direction, vec3& velocity) const
{
    auto const& turret = _design->turrets[turret_index];

    rot2 turret_rx = rot2(turret.orientation + _turrets[turret_index].traverse);
    mat4 turret_tx4 = mat4(turret_rx[0], turret_rx[1], 0, 0,
                          -turret_rx[1], turret_rx[0], 0, 0,
                           0,             0,           1, 0,
                           turret.position.x, turret.position.y, turret.position.z, 1) * get_transform();

    double cp = cos(_turrets[turret_index].elevation);
    double sp = sin(_turrets[turret_index].elevation);

    // TODO: fix hard-coded muzzle transform
    vec3 gun_offset = turret.design->position[gun_index];
    vec3 muzzle_offset = vec3(0.9 * turret.design->gun_design->length, 0, 0);

    mat4 gun_tx4 = mat4(cp, 0, sp, 0,
                        0,  1, 0,  0,
                       -sp, 0, cp, 0,
                        gun_offset.x, gun_offset.y, gun_offset.z, 1) * turret_tx4;

    position = muzzle_offset * gun_tx4;
    direction = vec3(cp, 0, sp) * turret_tx4.submatrix<3,3>();
    velocity = get_linear_velocity() + cross(get_angular_velocity(), position - get_position());
}

} // namespace game
