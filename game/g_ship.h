// g_ship.h
//

#pragma once

#include "g_object.h"
#include "g_ship_layout.h"
#include "p_compound.h"
#include "r_model.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class character;
class engines;
class shield;
class weapon;
class subsystem;

//------------------------------------------------------------------------------
struct ship_info
{
    std::string name;
    render::model model; //!< rendering model
    physics::compound_shape shape; //!< physics shape
    game::ship_layout layout; //!< compartment layout
    std::vector<vec2> hardpoints; //!< weapon hardpoints
};

//------------------------------------------------------------------------------
class ship : public object
{
public:
    static const object_type _type;

public:
    ship(ship_info const& info);
    ~ship();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual bool touch(object *other, physics::collision const* collision) override;
    virtual void think() override;

    virtual void read_snapshot(network::message const& message) override;
    virtual void write_snapshot(network::message& message) const override;

    virtual void set_position(vec2 position, bool teleport = false) override;
    virtual void set_rotation(float rotation, bool teleport = false) override;

    void damage(object* inflictor, vec2 point, float amount);

    std::vector<unique_handle<character>>& crew() { return _crew; }
    std::vector<unique_handle<character>> const& crew() const { return _crew; }

    std::vector<unique_handle<subsystem>>& subsystems() { return _subsystems; }
    std::vector<unique_handle<subsystem>> const& subsystems() const { return _subsystems; }

    ship_layout const& layout() const { return _state.layout(); }
    ship_state& state() { return _state; }

    handle<subsystem> reactor() { return _reactor; }
    handle<game::engines> engines() { return _engines; }
    handle<game::engines const> engines() const { return _engines; }
    handle<game::shield> shield() { return _shield; }
    handle<game::shield const> shield() const { return _shield; }
    std::vector<handle<game::weapon>>& weapons() { return _weapons; }
    std::vector<handle<game::weapon>> const& weapons() const { return _weapons; }

    bool is_destroyed() const { return _is_destroyed; }

    static ship_info const& by_random(random& r);

protected:
    string::buffer _name;
    ship_info const& _info;

    std::vector<unique_handle<character>> _crew;
    std::vector<unique_handle<subsystem>> _subsystems;

    ship_state _state;

    handle<subsystem> _reactor;
    handle<game::engines> _engines;
    handle<game::shield> _shield;
    std::vector<handle<game::weapon>> _weapons;

    time_value _dead_time;

    bool _is_destroyed;

    static constexpr time_delta destruction_time = time_delta::from_seconds(9.f);
    static constexpr time_delta respawn_time = time_delta::from_seconds(3.f);

    static physics::material _material;

    static const std::vector<ship_info> _types;
};

} // namespace game
