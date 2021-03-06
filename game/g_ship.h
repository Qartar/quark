// g_ship.h
//

#pragma once

#include "g_object.h"
#include "p_compound.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class character;
class engines;
class shield;
class weapon;
class subsystem;

//------------------------------------------------------------------------------
class ship : public object
{
public:
    static const object_type _type;

public:
    ship();
    ~ship();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    void update_usercmd(game::usercmd usercmd);
    void damage(object* inflictor, vec2 point, float amount);

    std::vector<unique_handle<subsystem>>& subsystems() { return _subsystems; }
    std::vector<unique_handle<subsystem>> const& subsystems() const { return _subsystems; }

    handle<subsystem> reactor() { return _reactor; }
    handle<game::engines> engines() { return _engines; }
    handle<game::engines const> engines() const { return _engines; }
    handle<game::shield> shield() { return _shield; }
    handle<game::shield const> shield() const { return _shield; }
    std::vector<handle<weapon>>& weapons() { return _weapons; }
    std::vector<handle<weapon>> const& weapons() const { return _weapons; }

    bool is_destroyed() const { return _is_destroyed; }

protected:
    game::usercmd _usercmd;

    std::vector<unique_handle<character>> _crew;
    std::vector<unique_handle<subsystem>> _subsystems;

    handle<subsystem> _reactor;
    handle<game::engines> _engines;
    handle<game::shield> _shield;
    std::vector<handle<weapon>> _weapons;

    time_value _dead_time;

    bool _is_destroyed;

    static constexpr time_delta destruction_time = time_delta::from_seconds(3.f);
    static constexpr time_delta respawn_time = time_delta::from_seconds(3.f);

    static physics::material _material;
    physics::compound_shape _shape;
};

} // namespace game
