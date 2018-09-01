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
class ship_layout
{
public:
    static constexpr uint16_t invalid_compartment = UINT16_MAX;

    struct compartment {
        uint16_t first_vertex;
        uint16_t num_vertices;
        // derived properties
        float area;
    };

    struct connection {
        uint16_t compartments[2];
        uint16_t vertices[4];
        // derived properties
        float width;
    };

    template<int num_vertices, int num_compartments, int num_connections>
    ship_layout(vec2 const (&vertices)[num_vertices],
                compartment const (&compartments)[num_compartments],
                connection const (&connections)[num_connections])
        : ship_layout(vertices, num_vertices, compartments, num_compartments, connections, num_connections)
    {}

    ship_layout(vec2 const* vertices, int num_vertices,
                compartment const* compartments, int num_compartments,
                connection const* connections, int num_connections);

    std::vector<vec2> const& vertices() const { return _vertices; }
    std::vector<compartment> const& compartments() const { return _compartments; }
    std::vector<connection> const& connections() const { return _connections; }

    uint16_t intersect_compartment(vec2 point) const;

protected:
    std::vector<vec2> _vertices;
    std::vector<compartment> _compartments;
    std::vector<connection> _connections;
};

//------------------------------------------------------------------------------
class ship_state
{
public:
    struct compartment_state {
        float atmosphere;
        float damage;
        float flow[2];

        float history[256];
    };

    struct connection_state {
        bool opened;
        float gradient;
        float flow;
        float velocity;
    };

    ship_state(ship_layout const& layout);

    void think();
    void recharge(float atmosphere_per_second);

    void set_connection(uint16_t index, bool opened) {
        _connections[index].opened = opened;
    }

    ship_layout const& layout() const { return _layout; }
    std::vector<compartment_state> const& compartments() const { return _compartments; }
    std::vector<connection_state> const& connections() const { return _connections; }

    int framenum;
protected:
    ship_layout const& _layout;
    std::vector<compartment_state> _compartments;
    std::vector<connection_state> _connections;
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
    handle<game::shield> shield() { return _shield; }
    handle<game::shield const> shield() const { return _shield; }
    std::vector<handle<weapon>>& weapons() { return _weapons; }
    std::vector<handle<weapon>> const& weapons() const { return _weapons; }

    bool is_destroyed() const { return _is_destroyed; }

protected:
    game::usercmd _usercmd;

public:
    ship_state _state;

protected:
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
