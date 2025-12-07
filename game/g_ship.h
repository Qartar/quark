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
class navigation;
class subsystem;

//------------------------------------------------------------------------------
struct turret_info
{
    vec2 position; //!< Position of the turret on the ship
    float radius; //!< Radius of the turret ring
    float orientation; //!< Default orientation, in radians from ship ahead
    vec2 traverse; //!< Minimuim and maximum traverse angle, in radians from default orientation
    float traverse_speed; //!< Angular speed in radians/sec

    int num_guns; //!< Number of gun barrels
    float spacing; //!< Spacing between each gun barrel
    float caliber; //!< Internal diameter of gun barrels
    float length; //!< Length of gun barrels
};

//------------------------------------------------------------------------------
struct ship_info
{
    string::buffer name;

    float length;
    float beam;

    float displacement;

    physics::compound_shape shape;
    std::vector<vec2> outline;

    std::vector<turret_info> turrets;
};

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
    handle<game::navigation> navigation() { return _navigation; }
    handle<game::navigation const> navigation() const { return _navigation; }

    bool is_destroyed() const { return _is_destroyed; }

protected:
    game::usercmd _usercmd;

    std::vector<unique_handle<character>> _crew;
    std::vector<unique_handle<subsystem>> _subsystems;

    handle<subsystem> _reactor;
    handle<game::engines> _engines;
    handle<game::navigation> _navigation;

    ship_info const* _info;

    struct turret_state {
        float traverse; //!< Current traverse angle in radians, relative to default orientation
        float elevation; //!< Current elevation angle in radians
    };

    std::vector<turret_state> _turrets;

    time_value _dead_time;

    bool _is_destroyed;

    static constexpr time_delta destruction_time = time_delta::from_seconds(3.f);
    static constexpr time_delta respawn_time = time_delta::from_seconds(3.f);

    static physics::material _material;
};

} // namespace game
