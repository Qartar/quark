// g_weapon.h
//

#pragma once

#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class shield;
class ship;

struct weapon_info
{
    weapon_type type;

    float projectile_speed; //!< launch speed of projectiles
    time_delta projectile_delay; //!< time between each projectile in an attack
    int projectile_count; //!< number of projectiles in each attack
    float projectile_damage; //!< damage per projectile
    bool projectile_inertia; //!< projectile inherits owner velocity

    time_delta beam_duration; //!< duration of beam attack
    float beam_sweep; //!< length of beam sweep
    float beam_damage; //!< beam damage per second

    time_delta reload_time; //!< time between firing
};

//------------------------------------------------------------------------------
class weapon : public subsystem
{
public:
    weapon(game::ship* owner, weapon_info const& info, vec2 position);
    virtual ~weapon();

    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    weapon_info const& info() const { return _info; }

    void attack_projectile(game::object* target, vec2 target_pos, bool repeat = false);
    void attack_beam(game::object* target, vec2 sweep_start, vec2 sweep_end, bool repeat = false);
    void cancel();

    object const* target() const { return _target.get(); }

protected:
    weapon_info _info;

    time_value _last_attack_time;

    game::handle<object> _target;
    vec2 _target_pos;
    vec2 _target_end;
    bool _is_attacking;
    bool _is_repeating;

    game::handle<object> _projectile_target;
    vec2 _projectile_target_pos;
    int _projectile_count;

    game::handle<object> _beam_target;
    vec2 _beam_sweep_start;
    vec2 _beam_sweep_end;
    game::handle<shield> _beam_shield; //!< beam weapons need to track whether it's hitting shields or not
};

} // namespace game
