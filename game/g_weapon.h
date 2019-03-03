// g_weapon.h
//

#pragma once

#include "g_subsystem.h"
#include "g_projectile.h"

#include <variant>

////////////////////////////////////////////////////////////////////////////////
namespace game {

class shield;
class ship;

//------------------------------------------------------------------------------
struct base_weapon_info
{
    string::literal name;
    time_delta reload_time; //!< time between firing
};

//------------------------------------------------------------------------------
struct projectile_weapon_info : base_weapon_info
{
    time_delta delay; //!< time between each projectile in an attack
    int count; //!< number of projectiles in each attack
    projectile_info projectile;
};

//------------------------------------------------------------------------------
struct beam_weapon_info : base_weapon_info
{
    time_delta duration; //!< duration of beam attack
    float sweep; //!< length of beam sweep
    float damage; //!< beam damage per second
    color4 color; //!< beam color
};

//------------------------------------------------------------------------------
using weapon_info = std::variant<projectile_weapon_info, beam_weapon_info>;

//------------------------------------------------------------------------------
class weapon : public subsystem
{
public:
    static const object_type _type;

public:
    weapon(game::ship* owner, weapon_info const& info, vec2 position);
    virtual ~weapon();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    weapon_info const& info() const { return _info; }

    void attack_projectile(game::object* target, vec2 target_pos, bool repeat = false);
    void attack_beam(game::object* target, vec2 sweep_start, vec2 sweep_end, bool repeat = false);
    void cancel();

    object const* target() const { return _target.get(); }
    bool is_attacking() const { return _is_attacking; }
    bool is_repeating() const { return _is_repeating; }

    static weapon_info const& by_random(random& r);

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

    static std::vector<weapon_info> _types;
};

} // namespace game
