// g_ship.h
//

#pragma once

#include "g_object.h"
#include "p_compound.h"
#include "cm_string.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class engines;
class navigation;
class subsystem;
class fire_director;

class faction;

struct ship_design;

//------------------------------------------------------------------------------
class ship : public object
{
public:
    static const object_type _type;

public:
    ship(handle<game::faction> faction);
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

    handle<game::engines> engines() { return _engines; }
    handle<game::engines const> engines() const { return _engines; }
    handle<game::navigation> navigation() { return _navigation; }
    handle<game::navigation const> navigation() const { return _navigation; }

    handle<game::faction> faction() const { return _faction; }

    ship_design const* design() const { return _design; }

protected:
    game::usercmd _usercmd;

    std::vector<unique_handle<subsystem>> _subsystems;

    handle<subsystem> _reactor;
    handle<game::engines> _engines;
    handle<game::navigation> _navigation;

    std::vector<handle<game::fire_director>> _fire_directors;

    handle<game::faction> _faction;

    ship_design const* _design;

    struct turret_state {
        double traverse; //!< Current traverse angle in radians, relative to default orientation
        double elevation; //!< Current elevation angle in radians

        double traverse_target;
        double elevation_target;

        time_value refire_time;

        handle<game::fire_director> fire_director;
    };

    std::vector<turret_state> _turrets;

    handle<ship const> _primary_target;

    vec2 _wake[128];
    std::size_t _wake_index;

    static physics::material _material;

protected:
    void update_targets();
    void update_firing_solution(std::size_t turret_index);

    //! Return the position, direction, and inertial velocity (i.e. inherited velocity) of the given turret/gun
    void get_firing_vectors(std::size_t turret_index, std::size_t gun_index, vec3& position, vec3& direction, vec3& inertial_velocity) const;
};

} // namespace game
