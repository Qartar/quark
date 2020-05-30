// g_ship_layout.h
//

#pragma once

#include "physics/p_shape.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class ship_layout
{
public:
    static constexpr uint16_t invalid_compartment = UINT16_MAX;
    static constexpr uint16_t invalid_connection = UINT16_MAX;

    struct compartment {
        uint16_t first_vertex;
        uint16_t num_vertices;
        // derived properties
        float area;
        physics::convex_shape shape;
        physics::convex_shape inner_shape;
    };

    struct connection {
        uint16_t compartments[2];
        // derived properties
        uint16_t vertices[2];
        float width;
    };

    template<int num_compartments, int num_connections>
    ship_layout(std::initializer_list<vec2> const (&compartments)[num_compartments],
                connection const (&connections)[num_connections])
        : ship_layout(compartments, num_compartments, connections, num_connections)
    {}

    ship_layout(std::initializer_list<vec2> const* compartments, int num_compartments,
                connection const* connections, int num_connections);

    std::vector<vec2> const& vertices() const { return _vertices; }
    std::vector<compartment> const& compartments() const { return _compartments; }
    std::vector<connection> const& connections() const { return _connections; }

    uint16_t intersect_compartment(vec2 point) const;
    std::size_t find_path(vec2 start, vec2 end, float radius, vec2* buffer, std::size_t buffer_size) const;

    //! Return a random point within the ship layout, uniformly distributed by area
    vec2 random_point(random& r) const;
    //! Return a random point within the given compartment, uniformly distributed by area
    vec2 random_point(random& r, uint16_t compartment) const;

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
        bool opened_automatic;
        float gradient;
        float flow;
        float velocity;
    };

    ship_state(ship_layout const& layout);

    void think();
    void recharge(float atmosphere_per_second);
    void repair(uint16_t index, float damage_per_second);

    void damage(uint16_t index, float amount) {
        _compartments[index].damage += amount;
    }

    void set_connection(uint16_t index, bool opened) {
        _connections[index].opened = opened;
    }

    void set_connection_automatic(uint16_t index, bool opened) {
        _connections[index].opened_automatic = opened;
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

} // namespace game
