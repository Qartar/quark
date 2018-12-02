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

//------------------------------------------------------------------------------
std::vector<weapon_info> weapon::_types = {
    projectile_weapon_info{
        /* name */              "blaster",
        /* type */              weapon_type::blaster,
        /* reload_time */       time_delta::from_seconds(8.f),
        /* speed */             786.f,
        /* delay */             time_delta::from_seconds(.2f),
        /* count */             3,
        /* damage */            .2f,
        /* inertia */           true,
    },
    projectile_weapon_info{
        /* name */              "cannon",
        /* type */              weapon_type::cannon,
        /* reload_time */       time_delta::from_seconds(8.f),
        /* speed */             1280.f,
        /* delay */             time_delta::from_seconds(.3f),
        /* count */             2,
        /* damage */            .3f,
        /* inertia */           true,
    },
    projectile_weapon_info{
        /* name */              "missile",
        /* type */              weapon_type::missile,
        /* reload_time */       time_delta::from_seconds(8.f),
        /* speed */             256.f,
        /* delay */             time_delta::from_seconds(.2f),
        /* count */             3,
        /* damage */            .2f,
        /* inertia */           true,
    },
    beam_weapon_info{
        /* name */              "laser",
        /* type */              weapon_type::laser,
        /* reload_time */       time_delta::from_seconds(8.f),
        /* duration */          time_delta::from_seconds(1.f),
        /* sweep */             1.5f,
        /* damage */            .4f,
    },
};

//------------------------------------------------------------------------------
weapon::weapon(game::ship* owner, weapon_info const& info, vec2 position)
    : subsystem(owner, {subsystem_type::weapons, 2})
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
{
    _type = object_type::weapon;
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
            vec2 beam_start = get_position(time) * mat3::rotate<2>(_owner->get_rotation(time)) + _owner->get_position(time);
            vec2 beam_end = (_beam_sweep_end * t + _beam_sweep_start * (1.f - t)) * mat3::rotate<2>(_beam_target->get_rotation(time)) + _beam_target->get_position(time);

            if (_beam_shield) {
                // Note: the traced rigid body does not use the interpolated position/rotation
                auto tr = physics::trace(&_beam_shield->rigid_body(), beam_start, beam_end);
                renderer->draw_line(2.f, beam_start, tr.get_contact().point, color4(1, 0.5, 0, 0.5), color4(1, 0, 0, 0.1f));
            } else {
                renderer->draw_line(2.f, beam_start, beam_end, color4(1, 0.5, 0, 0.5), color4(1, 0, 0, 0.1f));
            }
        }
    }
}

//------------------------------------------------------------------------------
void weapon::think()
{
    subsystem::think();

    // cancel pending attacks if weapon subsystem has been damaged
    if (current_power() < maximum_power()) {
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
                game::projectile* proj = get_world()->spawn<projectile>(_owner.get(), projectile_info.damage, projectile_info.type);
                vec2 start = get_position() * _owner->rigid_body().get_transform();
                vec2 end = _projectile_target_pos * _projectile_target->rigid_body().get_transform();

                vec2 relative_velocity = _projectile_target->get_linear_velocity();
                if (projectile_info.inertia) {
                    relative_velocity -= _owner->get_linear_velocity();
                }

                // lead target based on relative velocity
                float dt = intercept_time(end - start, relative_velocity, projectile_info.speed);
                if (dt > 0.f) {
                    end += relative_velocity * dt;
                }

                vec2 dir = (end - start).normalize();

                vec2 projectile_velocity = dir * projectile_info.speed;
                if (projectile_info.inertia) {
                    projectile_velocity += _owner->get_linear_velocity();
                }

                proj->set_position(start, true);
                proj->set_linear_velocity(projectile_velocity);
                proj->set_rotation(std::atan2(dir.y, dir.x), true);

                if (projectile_info.type == weapon_type::blaster) {
                    sound::asset _sound_blaster_fire = pSound->load_sound("assets/sound/blaster_fire.wav");
                    get_world()->add_sound(_sound_blaster_fire, start, projectile_info.damage);
                    get_world()->add_effect(time, effect_type::blaster, start, dir * 2);
                } else if (projectile_info.type == weapon_type::cannon) {
                    sound::asset _sound_cannon_fire = pSound->load_sound("assets/sound/cannon_fire.wav");
                    get_world()->add_sound(_sound_cannon_fire, start, projectile_info.damage);
                    get_world()->add_effect(time, effect_type::cannon, start, dir * 2);
                } else {
                    get_world()->add_effect(time, effect_type::cannon, start, dir * 2);
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
            if (obj && obj->_type == object_type::shield && obj->touch(this, &c)) {
                _beam_shield = static_cast<game::shield*>(obj);
            } else {
                _beam_shield = nullptr;
                get_world()->add_effect(time, effect_type::sparks, beam_end, -beam_dir, beam_info.damage);
                if (_beam_target->_type == object_type::ship) {
                    static_cast<ship*>(_beam_target.get())->damage(this, beam_end, beam_info.damage * FRAMETIME.to_seconds());
                }
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
void weapon::attack_projectile(game::object* target, vec2 target_pos, bool repeat)
{
    _target = target;
    _target_pos = target_pos;// / target->rigid_body.get_transform();
    _is_attacking = true;
    _is_repeating = repeat;
}

//------------------------------------------------------------------------------
void weapon::attack_beam(game::object* target, vec2 sweep_start, vec2 sweep_end, bool repeat)
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
    _is_attacking = false;
}

//------------------------------------------------------------------------------
weapon_info const& weapon::by_random(random& r)
{
    return _types[r.uniform_int(_types.size())];
}

} // namespace game
