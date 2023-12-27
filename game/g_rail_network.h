// g_rail_network.h
//

#pragma once

#include "cm_clothoid_network.h"
#include "cm_string.h"
#include "g_object.h"

#include <map>

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

    static constexpr float track_clearance = 5.f;
    static constexpr float junction_clearance = 4.f;

public:
    rail_network(world* w);
    virtual ~rail_network();

    void clear();

    void draw(render::system* renderer, time_value time) const;

    void add_segment(clothoid::segment s);
    handle<rail_signal> add_signal(vec2 position);
    handle<rail_station> add_station(vec2 position, string::view name);

    clothoid::segment get_segment(clothoid::network::edge_index edge) const;

    clothoid::network::node_index start_node(clothoid::network::edge_index edge) const;

    float get_clearance(clothoid::network::edge_index from, clothoid::network::edge_index to) const;

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

    using edge_pair = std::pair<clothoid::network::edge_index, clothoid::network::edge_index>;
    std::map<edge_pair, float> _clearance;

protected:
    bool calculate_clearance(clothoid::network::edge_index e0, clothoid::network::edge_index e1, float clearance, float& c0, float& c1) const;
    void update_clearance(clothoid::network::edge_index edge);
};

//------------------------------------------------------------------------------
inline float rail_network::get_clearance(clothoid::network::edge_index from, clothoid::network::edge_index to) const
{
    auto it = _clearance.find(std::make_pair(from, to));
    return it != _clearance.end() ? it->second : 0.f;
}

} // namespace game
