// g_weapon.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_weapon.h"
#include "cm_geometry.h"
#include "g_projectile.h"
#include "g_shield.h"
#include "g_ship.h"
#include "p_collide.h"
#include "p_trace.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type weapon::_type(subsystem::_type);

//------------------------------------------------------------------------------
const std::vector<weapon_info> weapon::_types = {
    projectile_weapon_info{
        /* name */              "blaster",
        /* reload_time */       time_delta::from_seconds(8.f),
        /* delay */             time_delta::from_seconds(.2f),
        /* count */             3,
        /* projectile */        {
            /* damage */            .2f,
            /* speed */             786.f,
            /* inertia */           true,
            /* homing */            false,
            /* fuse_time */         time_delta::from_seconds(5.f),
            /* fade_time */         time_delta::from_seconds(1.f),
            /* color */             color4(1.f, 0.f, 0.f, -1.f),
            /* tail_time */         time_delta::from_seconds(.02f),
            /* launch_effect */     effect_type::blaster,
            /* launch_sound */      sound::asset::invalid,
            /* flight_effect */     effect_type::none,
            /* flight sound */      sound::asset::invalid,
            /* impact_effect */     effect_type::blaster_impact,
            /* impact_sound */      sound::asset::invalid,
                                },
    },
    projectile_weapon_info{
        /* name */              "cannon",
        /* reload_time */       time_delta::from_seconds(8.f),
        /* delay */             time_delta::from_seconds(.3f),
        /* count */             2,
        /* projectile */        {
            /* damage */            .3f,
            /* speed */             1280.f,
            /* inertia */           true,
            /* homing */            false,
            /* fuse_time */         time_delta::from_seconds(5.f),
            /* fade_time */         time_delta::from_seconds(1.f),
            /* color */             color4(1.f, .5f, 0.f, -1.f),
            /* tail_time */         time_delta::from_seconds(.02f),
            /* launch_effect */     effect_type::cannon,
            /* launch_sound */      sound::asset::invalid,
            /* flight_effect */     effect_type::none,
            /* flight sound */      sound::asset::invalid,
            /* impact_effect */     effect_type::cannon_impact,
            /* impact_sound */      sound::asset::invalid,
                                },
    },
    projectile_weapon_info{
        /* name */              "missile",
        /* reload_time */       time_delta::from_seconds(8.f),
        /* delay */             time_delta::from_seconds(.2f),
        /* count */             3,
        /* projectile */        {
            /* damage */            .2f,
            /* speed */             256.f,
            /* inertia */           true,
            /* homing */            true,
            /* fuse_time */         time_delta::from_seconds(5.f),
            /* fade_time */         time_delta::from_seconds(1.f),
            /* color */             color4(1.f, 1.f, 1.f, 1.f),
            /* tail_time */         time_delta::from_seconds(.02f),
            /* launch_effect */     effect_type::none,
            /* launch_sound */      sound::asset::invalid,
            /* flight_effect */     effect_type::missile_trail,
            /* flight sound */      sound::asset::invalid,
            /* impact_effect */     effect_type::missile_impact,
            /* impact_sound */      sound::asset::invalid,
                                },
    },
    beam_weapon_info{
        /* name */              "laser",
        /* reload_time */       time_delta::from_seconds(8.f),
        /* duration */          time_delta::from_seconds(1.f),
        /* sweep */             1.5f,
        /* damage */            .4f,
        /* color */             color4(1.f, .5f, 0.f, -1.f),
    },
    pulse_weapon_info{
        /* name */              "phaser",
        /* reload_time */       time_delta::from_seconds(8.f),
        /* delay */             time_delta::from_seconds(.3f),
        /* count */             2,
        /* damage */            .3f,
        /* color */             color4(1.f, .2f, 0.f, -1.f),
        /* launch_effect */     effect_type::none,
        /* launch_sound */      sound::asset::invalid,
        /* impact_effect */     effect_type::none,
        /* impact_sound */      sound::asset::invalid,
    },
};

//------------------------------------------------------------------------------
weapon::weapon(game::ship* owner, uint16_t compartment, weapon_info const& info, vec2 position)
    : subsystem(owner, compartment, {subsystem_type::weapons, 2})
    , _info(info)
    , _last_attack_time(time_value::zero)
    , _target(nullptr)
    , _target_pos(vec2_zero)
    , _target_end(vec2_zero)
    , _is_attacking(false)
    , _is_repeating(false)
    , _projectile_target(nullptr)
    , _projectile_target_pos(vec2_zero)
    , _projectile_count(0)
    , _beam_target(nullptr)
    , _beam_sweep_start(vec2_zero)
    , _beam_sweep_end(vec2_zero)
    , _beam_shield(nullptr)
    , _pulse_target(nullptr)
    , _pulse_target_pos(vec2_zero)
    , _pulse_count(0)
{
    set_position(position, true);
}

//------------------------------------------------------------------------------
weapon::~weapon()
{
}

//------------------------------------------------------------------------------
void weapon::draw(render::system* renderer, time_value time) const
{
    if (std::holds_alternative<beam_weapon_info>(_info)) {
        auto& beam_info = std::get<beam_weapon_info>(_info);

        if (_beam_target && time - _last_attack_time < beam_info.duration) {
            float t = (time - _last_attack_time) / beam_info.duration;
            vec2 beam_start = get_position(time) * _owner->get_transform(time);
            vec2 beam_end = (_beam_sweep_end * t + _beam_sweep_start * (1.f - t)) * _beam_target->get_transform(time);

            color4 core_color = beam_info.color * color4(1.f, 1.f, 1.f, .5f);
            color4 edge_color = beam_info.color * color4(1.f, 1.f, 1.f, .1f);

            if (_beam_shield) {
                // Trace rigid body using the interpolated position/rotation
                physics::rigid_body shield_proxy = _beam_shield->rigid_body();
                shield_proxy.set_position(_beam_shield->get_position(time));
                shield_proxy.set_rotation(_beam_shield->get_rotation(time));
                auto tr = physics::trace(&shield_proxy, beam_start, beam_end);
                renderer->draw_line(2.f, beam_start, tr.get_contact().point, core_color, edge_color);
            } else {
                renderer->draw_line(2.f, beam_start, beam_end, core_color, edge_color);
            }
        }
    }

    if (std::holds_alternative<pulse_weapon_info>(_info)) {
        auto& pulse_info = std::get<pulse_weapon_info>(_info);

        time_value last_pulse_time = _last_attack_time + pulse_info.delay * (_pulse_count - 1);

        if (_pulse_target && time - last_pulse_time < pulse_info.delay) {
            float t = (time - last_pulse_time) / pulse_info.delay;
            vec2 start = get_position(time) * _owner->get_transform(time);
            vec2 end = _pulse_target_pos * _pulse_target->get_transform(time);

            float a = std::exp(-5.f * t);

            color4 core_color = pulse_info.color * color4(1.f, 1.f, 1.f, .5f * a);
            color4 edge_color = pulse_info.color * color4(1.f, 1.f, 1.f, .1f * a);

            if (_pulse_shield) {
                // Trace rigid body using the interpolated position/rotation
                physics::rigid_body shield_proxy = _pulse_shield->rigid_body();
                shield_proxy.set_position(_pulse_shield->get_position(time));
                shield_proxy.set_rotation(_pulse_shield->get_rotation(time));
                auto tr = physics::trace(&shield_proxy, start, end);
                renderer->draw_line(2.f, start, tr.get_contact().point, core_color, edge_color);
            } else {
                renderer->draw_line(2.f, start, end, core_color, edge_color);
            }
        }
    }
}

//------------------------------------------------------------------------------
void weapon::think()
{
    subsystem::think();

    ship const* target_ship = _target && _target->is_type<ship>()
        ? static_cast<ship const*>(_target.get()) : nullptr;

    // cancel pending attacks if weapon subsystem has been damaged or if target
    // has been destroyed (ships are not immediately removed when destroyed)
    if (current_power() < maximum_power() || !_target
            || (target_ship && target_ship->is_destroyed())) {
        cancel();
    }

    time_value time = get_world()->frametime();

    //
    // update projectiles
    //

    if (std::holds_alternative<projectile_weapon_info>(_info)) {
        auto& projectile_info = std::get<projectile_weapon_info>(_info);

        if (_is_attacking && time - _last_attack_time > projectile_info.reload_time) {
            _projectile_target = _target;
            _projectile_target_pos = _target_pos;
            _projectile_count = 0;
            _last_attack_time = time;
            _is_attacking = _is_repeating;
        }

        if (_projectile_target && time - _last_attack_time <= projectile_info.count * projectile_info.delay) {
            if (_projectile_count < projectile_info.count && _projectile_count * projectile_info.delay <= time - _last_attack_time) {
                game::projectile* proj = get_world()->spawn<projectile>(_owner.get(), projectile_info.projectile, _random.uniform_real() < .8f ? _projectile_target : nullptr);
                vec2 start = get_position() * _owner->rigid_body().get_transform();
                vec2 end = _projectile_target_pos * _projectile_target->rigid_body().get_transform();

                vec2 relative_velocity = _projectile_target->get_linear_velocity();
                if (projectile_info.projectile.inertia) {
                    relative_velocity -= _owner->get_linear_velocity();
                }

                // lead target based on relative velocity
                float dt = intercept_time(end - start, relative_velocity, projectile_info.projectile.speed);
                if (dt > 0.f) {
                    end += relative_velocity * dt;
                }

                vec2 dir = (end - start).normalize();

                vec2 projectile_velocity = dir * projectile_info.projectile.speed;
                if (projectile_info.projectile.inertia) {
                    projectile_velocity += _owner->get_linear_velocity();
                }

                proj->set_position(start, true);
                proj->set_linear_velocity(projectile_velocity);
                proj->set_rotation(std::atan2(dir.y, dir.x), true);

                if (projectile_info.projectile.launch_effect != effect_type::none) {
                    get_world()->add_effect(time, projectile_info.projectile.launch_effect, start, dir * 2);
                }

                if (projectile_info.projectile.launch_sound != sound::asset::invalid) {
                    get_world()->add_sound(projectile_info.projectile.launch_sound, start, projectile_info.projectile.damage);
                }

                ++_projectile_count;
            }
        }
    }

    //
    // update beam
    //

    if (std::holds_alternative<beam_weapon_info>(_info)) {
        auto& beam_info = std::get<beam_weapon_info>(_info);

        if (_is_attacking && time - _last_attack_time > beam_info.reload_time) {
            _beam_target = _target;
            _beam_sweep_start = _target_pos;
            _beam_sweep_end = _target_end;
            _beam_shield = nullptr;

            _last_attack_time = time;
            _is_attacking = _is_repeating;
        }

        if (_beam_target && time - _last_attack_time < beam_info.duration) {
            float t = (time - _last_attack_time) / beam_info.duration;
            vec2 beam_start = get_position() * _owner->rigid_body().get_transform();
            vec2 beam_end = (_beam_sweep_end * t + _beam_sweep_start * (1.f - t)) * _beam_target->rigid_body().get_transform();
            vec2 beam_dir = (beam_end - beam_start).normalize();

            physics::collision c;
            game::object* obj = get_world()->trace(c, beam_start, beam_end, _owner.get());
            if (obj && obj->is_type<shield>() && obj->touch(this, &c)) {
                _beam_shield = static_cast<game::shield*>(obj);
                _beam_shield->damage(c.point, beam_info.damage * FRAMETIME.to_seconds());
            } else {
                _beam_shield = nullptr;
                get_world()->add_effect(time, effect_type::sparks, beam_end, -beam_dir, beam_info.damage);
                if (_beam_target->is_type<ship>()) {
                    static_cast<ship*>(_beam_target.get())->damage(this, beam_end, beam_info.damage * FRAMETIME.to_seconds());
                }
            }
        }
    }

    //
    // update pulse
    //

    if (std::holds_alternative<pulse_weapon_info>(_info)) {
        auto& pulse_info = std::get<pulse_weapon_info>(_info);

        if (_is_attacking && time - _last_attack_time > pulse_info.reload_time) {
            _pulse_target = _target;
            _pulse_target_pos = _target_pos;
            _pulse_count = 0;
            _pulse_shield = nullptr;
            _last_attack_time = time;
            _is_attacking = _is_repeating;
        }

        if (_pulse_target && time - _last_attack_time <= pulse_info.count * pulse_info.delay) {
            if (_pulse_count < pulse_info.count && _pulse_count * pulse_info.delay <= time - _last_attack_time) {
                vec2 start = get_position() * _owner->rigid_body().get_transform();
                vec2 end = _pulse_target_pos * _pulse_target->rigid_body().get_transform();
                vec2 dir = (end - start).normalize();

                if (pulse_info.launch_effect != effect_type::none) {
                    get_world()->add_effect(time, pulse_info.launch_effect, start, dir * 2);
                }

                if (pulse_info.launch_sound != sound::asset::invalid) {
                    get_world()->add_sound(pulse_info.launch_sound, start, pulse_info.damage);
                }

                physics::collision c;
                game::object* obj = get_world()->trace(c, start, end, _owner.get());
                if (obj && obj->is_type<shield>() && obj->touch(this, &c)) {
                    _pulse_shield = static_cast<game::shield*>(obj);
                    if (pulse_info.impact_effect != effect_type::none) {
                        get_world()->add_effect(time, pulse_info.impact_effect, c.point, -dir, .5f * pulse_info.damage);
                    }
                    _pulse_shield->damage(c.point, pulse_info.damage);
                } else {
                    _pulse_shield = nullptr;
                    if (pulse_info.impact_effect != effect_type::none) {
                        get_world()->add_effect(time, pulse_info.impact_effect, end, -dir, pulse_info.damage);
                    }
                    if (_pulse_target->is_type<ship>()) {
                        static_cast<ship*>(_pulse_target.get())->damage(this, end, pulse_info.damage);
                    }
                }

                ++_pulse_count;
            }
        }
    }
}

//------------------------------------------------------------------------------
void weapon::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void weapon::write_snapshot(network::message& /*message*/) const
{
}

//------------------------------------------------------------------------------
void weapon::attack_point(game::object* target, vec2 target_pos, bool repeat)
{
    _target = target;
    _target_pos = target_pos;// / target->rigid_body.get_transform();
    _target_end = target_pos;// / target->rigid_body.get_transform();
    _is_attacking = true;
    _is_repeating = repeat;
}

//------------------------------------------------------------------------------
void weapon::attack_sweep(game::object* target, vec2 sweep_start, vec2 sweep_end, bool repeat)
{
    _target = target;
    _target_pos = sweep_start;// / target->rigid_body().get_transform();
    _target_end = sweep_end;// / target->rigid_body().get_transform();
    _is_attacking = true;
    _is_repeating = repeat;
}

//------------------------------------------------------------------------------
void weapon::cancel()
{
    _target = nullptr;
    _is_attacking = false;
    _is_repeating = false;
}

//------------------------------------------------------------------------------
weapon_info const& weapon::by_random(random& r)
{
    return _types[r.uniform_int(_types.size())];
}

} // namespace game
