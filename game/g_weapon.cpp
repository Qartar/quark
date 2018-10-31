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
    if (_beam_target && time - _last_attack_time < _info.beam_duration) {
        float t = (time - _last_attack_time) / _info.beam_duration;
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

//------------------------------------------------------------------------------
void weapon::think()
{
    subsystem::think();

    // cancel pending attacks if weapon subsystem has been damaged
    if (current_power() < maximum_power()) {
        cancel();
    }

    time_value time = get_world()->frametime();

    if (_is_attacking && time - _last_attack_time > _info.reload_time) {
        if (_info.type != weapon_type::laser) {
            _projectile_target = _target;
            _projectile_target_pos = _target_pos;
            _projectile_count = 0;
        } else {
            _beam_target = _target;
            _beam_sweep_start = _target_pos;
            _beam_sweep_end = _target_end;
            _beam_shield = nullptr;
        }
        _last_attack_time = time;
        _is_attacking = _is_repeating;
    }

    //
    // update projectiles
    //

    if (_projectile_target && time - _last_attack_time <= _info.projectile_count * _info.projectile_delay) {
        if (_projectile_count < _info.projectile_count && _projectile_count * _info.projectile_delay <= time - _last_attack_time) {
            game::projectile* proj = get_world()->spawn<projectile>(_owner.get(), _info.projectile_damage, _info.type);
            vec2 start = get_position() * _owner->rigid_body().get_transform();
            vec2 end = _projectile_target_pos * _projectile_target->rigid_body().get_transform();

            vec2 relative_velocity = _projectile_target->get_linear_velocity();
            if (_info.projectile_inertia) {
                relative_velocity -= _owner->get_linear_velocity();
            }

            // lead target based on relative velocity
            float dt = intercept_time(end - start, relative_velocity, _info.projectile_speed);
            if (dt > 0.f) {
                end += relative_velocity * dt;
            }

            vec2 dir = (end - start).normalize();

            vec2 projectile_velocity = dir * _info.projectile_speed;
            if (_info.projectile_inertia) {
                projectile_velocity += _owner->get_linear_velocity();
            }

            proj->set_position(start, true);
            proj->set_linear_velocity(projectile_velocity);
            proj->set_rotation(std::atan2(dir.y, dir.x), true);

            if (_info.type == weapon_type::blaster) {
                sound::asset _sound_blaster_fire = pSound->load_sound("assets/sound/blaster_fire.wav");
                get_world()->add_sound(_sound_blaster_fire, start, _info.projectile_damage);
                get_world()->add_effect(time, effect_type::blaster, start, dir * 2);
            } else if (_info.type == weapon_type::cannon) {
                sound::asset _sound_cannon_fire = pSound->load_sound("assets/sound/cannon_fire.wav");
                get_world()->add_sound(_sound_cannon_fire, start, _info.projectile_damage);
                get_world()->add_effect(time, effect_type::cannon, start, dir * 2);
            } else {
                get_world()->add_effect(time, effect_type::cannon, start, dir * 2);
            }

            ++_projectile_count;
        }
    }

    //
    // update beam
    //

    if (_beam_target && time - _last_attack_time < _info.beam_duration) {
        float t = (time - _last_attack_time) / _info.beam_duration;
        vec2 beam_start = get_position() * _owner->rigid_body().get_transform();
        vec2 beam_end = (_beam_sweep_end * t + _beam_sweep_start * (1.f - t)) * _beam_target->rigid_body().get_transform();
        vec2 beam_dir = (beam_end - beam_start).normalize();

        physics::collision c{};
        game::object* obj = get_world()->trace(c, beam_start, beam_end, _owner.get());
        if (obj && obj->_type == object_type::shield && obj->touch(this, &c)) {
            _beam_shield = static_cast<game::shield*>(obj);
        } else {
            _beam_shield = nullptr;
            get_world()->add_effect(time, effect_type::sparks, beam_end, -beam_dir, _info.beam_damage);
            if (_beam_target->_type == object_type::ship) {
                static_cast<ship*>(_beam_target.get())->damage(this, beam_end, _info.beam_damage * FRAMETIME.to_seconds());
            }
        }

        // damage
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

} // namespace game
