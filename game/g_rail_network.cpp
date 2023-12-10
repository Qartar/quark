// g_rail_network.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_rail_network.h"
#include "g_rail_station.h"

#include <set>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
rail_network::rail_network(world* w)
    : _world(w)
{
}

//------------------------------------------------------------------------------
rail_network::~rail_network()
{
}

//------------------------------------------------------------------------------
void rail_network::clear()
{
    _signals.clear();
    _signal_block_index.clear();
    _stations.clear();
    _trains.clear();

    _network = {};

    _signal_blocks.clear();
}

//------------------------------------------------------------------------------
void rail_network::draw(render::system* renderer, time_value time) const
{
    (void)time;
    render::view const& view = renderer->view();
    //float diag = view.size.length() / float(view.viewport.size().length());
    float diag = 16.f * view.size.length() / float(renderer->window()->size().length());
    for (auto edge : _network.edges()) {
        // skip opposite edges
        if (edge & 1) {
            continue;
        }
        clothoid::segment s = _network.get_segment(edge);
        if (s.type() == clothoid::segment_type::line) {
            renderer->draw_line(
                s.initial_position(),
                s.final_position(),
                color4(1,1,1,1),
                color4(1,1,1,1));
        } else {
            float n = max(2.f, s.length() / diag);
            vec2 p0 = s.initial_position();
            for (float ii = 1.f; ii < n; ii += 1.f) {
                vec2 p1 = s.evaluate(ii / n * s.length());
                renderer->draw_line(p0, p1, color4(1,1,1,1), color4(1,1,1,1));
                p0 = p1;
            }
            renderer->draw_line(p0, s.final_position(), color4(1,1,1,1), color4(1,1,1,1));
        }
    }
}

//------------------------------------------------------------------------------
void rail_network::add_segment(clothoid::segment s)
{
    _network.insert_edge(s);
}

//------------------------------------------------------------------------------
handle<rail_signal> rail_network::add_signal(vec2 position)
{
    (void)position;
    return handle<rail_signal>();
}

//------------------------------------------------------------------------------
handle<rail_station> rail_network::add_station(vec2 position, string::view name)
{
    clothoid::network::edge_index edge;
    float dist;
    if (get_closest_segment(position, 1.f, edge, dist)) {
        return _world->spawn<rail_station>(edge, dist, name);
    } else {
        return handle<rail_station>();
    }
}

//------------------------------------------------------------------------------
clothoid::segment rail_network::get_segment(clothoid::network::edge_index edge) const
{
    return _network.get_segment(edge);
}

//------------------------------------------------------------------------------
bool rail_network::get_closest_segment(
    vec2 position,
    float max_distance,
    clothoid::network::edge_index& edge,
    float& length) const
{
    return _network.get_closest_segment(
        position,
        max_distance,
        edge,
        length);
}

//------------------------------------------------------------------------------
std::size_t rail_network::find_path(rail_position start, rail_position goal, edge_index* edges, std::size_t max_edges) const
{
    vec2 goal_pos;
    if (goal.is_edge) {
        goal_pos = _network.get_segment(goal.edge).evaluate(goal.dist);
    } else if (goal.is_node) {
        goal_pos = _network.node_position(goal.node);
    }

    struct search_state {
        float distance; //!< total distance along path, including this edge
        float heuristic; //!< estimated distance to destination
        std::size_t previous; //!< index of previous search node
        node_index node; //!< destination node
        edge_index edge; //!< traversed edge
    };

    std::vector<search_state> search;

    if (start.is_edge) {
        vec2 start_pos = _network.get_segment(start.edge).evaluate(start.dist);
        search.push_back({
            0.f,
            (start_pos - goal_pos).length(),
            SIZE_MAX,
            _network.end_node(start.edge),
            start.edge,
        });
    } else if (start.is_node) {
        // populate initial search state with all edges connected to start node
        for (edge_index edge = _network.first_edge(start.node)
            ; edge != clothoid::network::invalid_edge
            ; edge = _network.next_edge(edge)) {

            node_index node = _network.end_node(edge);

            search.push_back({
                _network.edge_length(edge),
                (goal_pos - _network.node_position(node)).length(),
                SIZE_MAX,
                node,
                edge,
                });
        }
    }

    struct search_comparator {
        decltype (search) const& _search;
        bool operator()(std::size_t lhs, std::size_t rhs) const {
            float d1 = _search[lhs].distance + _search[lhs].heuristic;
            float d2 = _search[rhs].distance + _search[rhs].heuristic;
            return d1 < d2;
        }
    };

    std::priority_queue<std::size_t, std::vector<std::size_t>, search_comparator> queue{{search}};
    std::set<std::size_t> closedset;

    for (std::size_t ii = 0; ii < search.size(); ++ii) {
        closedset.insert(search[ii].edge);
        queue.push(ii);
    }

    // drain the queue until we reach the goal or run out of edges
    while (queue.size()) {
        std::size_t idx = queue.top(); queue.pop();

        // check if search reached the goal
        if (goal.is_edge && search[idx].edge == goal.edge
            || goal.is_node && search[idx].node == goal.node) {

            // determine the length of the path by backtracking through the search state
            std::size_t depth = 0;
            for (std::size_t ii = idx; search[ii].previous != SIZE_MAX; ii = search[ii].previous) {
                ++depth;
            }

            // if path buffer is too small then just return the path size
            if (depth > max_edges) {
                return depth;
            }

            // fill in the path buffer by backtracking through the search state
            edge_index* edge_ptr = edges + depth - 1;
            for (std::size_t ii = idx; search[ii].previous != SIZE_MAX; ii = search[ii].previous) {
                *edge_ptr-- = search[ii].edge;
            }
            return depth;
        }

        // add all suitable edges from the search node to the queue
        vec2 dir = _network.get_segment(search[idx].edge).final_tangent();
        for (edge_index edge = _network.first_edge(search[idx].node)
            ; edge != clothoid::network::invalid_edge
            ; edge = _network.next_edge(edge)) {

            if (dot(dir, _network.edge_direction(edge)) < .999f) {
                continue;
            }

            if (closedset.find(edge) != closedset.end()) {
                continue;
            }

            node_index node = _network.end_node(edge);

            search.push_back({
                search[idx].distance + _network.edge_length(edge),
                (goal_pos - _network.node_position(node)).length(),
                idx,
                node,
                edge,
            });
            queue.push(search.size() - 1);
            closedset.insert(edge);
        }
    }

    // search failed
    return 0;
}

} // namespace game
