// g_ship.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class shield;
class weapon;
class subsystem;

//------------------------------------------------------------------------------
class ship : public object
{
public:
    ship();
    ~ship();

    void spawn();

    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    void update_usercmd(game::usercmd usercmd);
    void damage(object* inflictor, vec2 point, float amount);

    std::vector<subsystem*> const& subsystems() const { return _subsystems; }

    bool is_destroyed() const { return _is_destroyed; }

protected:
    game::usercmd _usercmd;

    std::vector<subsystem*> _subsystems;

    handle<subsystem> _reactor;
    handle<subsystem> _engines;
    handle<shield> _shield;
    std::vector<handle<weapon>> _weapons;

    time_value _dead_time;

    bool _is_destroyed;

    static constexpr time_delta destruction_time = time_delta::from_seconds(3.f);
    static constexpr time_delta respawn_time = time_delta::from_seconds(3.f);

    static physics::material _material;
    physics::convex_shape _shape;
};

} // namespace game
