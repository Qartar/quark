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
class rail_network
{
public:
    rail_network(world* w);
    virtual ~rail_network();

    void draw(render::system* renderer, time_value time) const;

    void add_segment(clothoid::segment s);
    handle<rail_signal> add_signal(vec2 position);
    handle<rail_station> add_station(vec2 position, string::view name);

    clothoid::segment get_segment(clothoid::network::edge_index edge) const;

    bool get_closest_segment(vec2 position, float max_distance, clothoid::network::edge_index& edge, float& length) const;

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
