// g_rail_network.h
//

#pragma once

#include "cm_clothoid_network.h"
#include "cm_string.h"
#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class rail_signal;
class rail_station;
class train;

//------------------------------------------------------------------------------
struct rail_position
{
    using edge_index = int;
    using node_index = int;
    bool is_edge : 1,
         is_node : 1;
    node_index node;
    edge_index edge;
    float dist;

    static rail_position from_edge(edge_index edge, float dist) {
        return {true, false, -1, edge, dist};
    }
    static rail_position from_node(node_index node) {
        return {false, true, node, -1, 0};
    }
};

//------------------------------------------------------------------------------
class rail_network
{
public:
    using edge_index = int;
    using node_index = int;

public:
    rail_network(world* w);
    virtual ~rail_network();

    void draw(render::system* renderer, time_value time) const;

    void add_segment(clothoid::segment s);
    handle<rail_signal> add_signal(vec2 position);
    handle<rail_station> add_station(vec2 position, string::view name);

    clothoid::segment get_segment(clothoid::network::edge_index edge) const;

    bool get_closest_segment(vec2 position, float max_distance, clothoid::network::edge_index& edge, float& length) const;

    std::size_t find_path(rail_position start, rail_position goal, edge_index* edges, std::size_t max_edges) const;

protected:
    world* _world;

    std::vector<handle<rail_signal>> _signals;
    std::vector<std::size_t> _signal_block_index;
    std::vector<handle<rail_station>> _stations;
    std::vector<handle<train>> _trains;

    clothoid::network _network;

    struct signal_block {
        std::vector<handle<rail_signal>> entrances;
        std::vector<handle<rail_signal>> exits;
    };

    std::vector<signal_block> _signal_blocks;
};

} // namespace game
