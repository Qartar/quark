// cm_clothoid_network.cpp
//

#include "cm_clothoid_network.h"
#include "cm_shared.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace clothoid {

//------------------------------------------------------------------------------
network::edge_index network::insert_edge(segment s)
{
    vec2 p0 = s.initial_position();
    vec2 p1 = s.final_position();

    node_index n0 = invalid_node;
    node_index n1 = invalid_node;

    // check for existing nodes at both ends of the segment
    for (node_index ii : nodes()) {
        if (length_sqr(_nodes[ii].position - p0) < epsilon_sqr) {
            assert(n0 == invalid_node); // multiple nodes within epsilon
            n0 = ii;
        }
        if (length_sqr(_nodes[ii].position - p1) < epsilon_sqr) {
            assert(n1 == invalid_node); // multiple nodes within epsilon
            n1 = ii;
        }
    }

    //
    // check for existing edges
    //

    if (n0 != invalid_node) {
        edge_index ei = _nodes[n0].first_edge;
        do {
            if (end_node(ei) == n1) {
                // TODO
            }
            ei = next_edge(ei);
        } while (ei != invalid_edge);
    }

    if (n1 != invalid_node) {
        edge_index ei = _nodes[n1].first_edge;
        do {
            if (start_node(ei) == n0) {
                // TODO
            }
            ei = next_edge(ei);
        } while (ei != invalid_edge);
    }

    //
    // allocate nodes if necessary
    //

    if (n0 == invalid_node) {
        n0 = alloc_node();
        _nodes[n0] = {p0, invalid_edge};
    }

    if (n1 == invalid_node) {
        n1 = alloc_node();
        _nodes[n1] = {p1, invalid_edge};
    }

    //
    // allocate edges and insert into graph
    //

    edge_index ei = alloc_edges();
    _edges[ei + 0] = {n1, invalid_edge};
    _edges[ei + 1] = {n0, invalid_edge};
    _segments[ei + 0] = s;
    _segments[ei + 1] = s.reverse();

    node_insert_edge(n0, ei + 0);
    node_insert_edge(n1, ei + 1);

    //
    // check for intersecting segments
    //

    // TODO

    return ei;
}

//------------------------------------------------------------------------------
void network::remove_edge(edge_index e)
{
    assert(e != invalid_edge);
    node_index n0 = start_node(e);
    node_index n1 = end_node(e);

    // disconnect edges
    node_remove_edge(n0, e);
    node_remove_edge(n1, e ^ 1);

    // free edges
    free_edges(e & ~1);

    // free disconnected nodes
    if (_nodes[n0].first_edge == invalid_edge) {
        free_node(n0);
    }
    if (_nodes[n1].first_edge == invalid_edge) {
        free_node(n1);
    }
}

//------------------------------------------------------------------------------
network::node_index network::insert_node(vec2 position)
{
    (void)position;
    return invalid_node; // TODO
}

//------------------------------------------------------------------------------
bool network::merge_node(node_index n)
{
    (void)n;
    return false; // TODO
}

//------------------------------------------------------------------------------
void network::remove_node(node_index n)
{
    (void)n;
    return; // TODO
}

//------------------------------------------------------------------------------
void network::node_insert_edge(node_index n, edge_index e)
{
    assert(n != invalid_node);
    assert(e != invalid_edge);
    _edges[e].next_edge = _nodes[n].first_edge;
    _nodes[n].first_edge = e;
}

//------------------------------------------------------------------------------
void network::node_remove_edge(node_index n, edge_index e)
{
    assert(n != invalid_node);
    assert(e != invalid_edge);
    if (_nodes[n].first_edge == e) {
        _nodes[n].first_edge = _edges[e].next_edge;
    } else {
        edge_index edge = _nodes[n].first_edge;
        while (edge != invalid_edge) {
            edge_index next = _edges[edge].next_edge;
            if (next == e) {
                _edges[edge].next_edge = _edges[e].next_edge;
                break;
            }
            edge = next;
        }
    }
}

//------------------------------------------------------------------------------
network::node_index network::alloc_node()
{
    if (_free_nodes.size()) {
        node_index n = _free_nodes.back();
        _free_nodes.pop_back();
        return n;
    } else {
        _nodes.push_back({});
        return narrow_cast<node_index>(_nodes.size() - 1);
    }
}

//------------------------------------------------------------------------------
void network::free_node(node_index n)
{
    assert(n != invalid_node);
    _free_nodes.push_back(n);
}

//------------------------------------------------------------------------------
network::edge_index network::alloc_edges()
{
    if (_free_edges.size()) {
        edge_index e = _free_edges.back();
        _free_edges.pop_back();
        return e;
    } else {
        _edges.push_back({});
        _edges.push_back({});
        _segments.push_back({});
        _segments.push_back({});
        assert(_edges.size() == _segments.size());
        return narrow_cast<edge_index>(_edges.size() - 2);
    }
}

//------------------------------------------------------------------------------
void network::free_edges(edge_index e)
{
    assert(e != invalid_edge);
    assert((e & 1) == 0);
    _free_edges.push_back(e);
}

//------------------------------------------------------------------------------
network::edge_index network::next_edge(edge_index e) const
{
    assert(e != invalid_edge);
    return _edges[e].next_edge;
}

//------------------------------------------------------------------------------
network::node_index network::start_node(edge_index e) const
{
    assert(e != invalid_edge);
    return _edges[e ^ 1].end_node;
}

//------------------------------------------------------------------------------
network::node_index network::end_node(edge_index e) const
{
    assert(e != invalid_edge);
    return _edges[e].end_node;
}

//------------------------------------------------------------------------------
vec2 network::edge_direction(edge_index e) const
{
    assert(e != invalid_edge);
    return _segments[e].initial_tangent();
}

//------------------------------------------------------------------------------
network::node_range network::nodes() const {
    return node_range{
        0,
        narrow_cast<node_index>(_nodes.size()),
        _free_nodes.data(),
        _free_nodes.data() + _free_nodes.size(),
    };
}

//------------------------------------------------------------------------------
network::edge_range network::edges() const {
    return edge_range{
        0,
        narrow_cast<node_index>(_edges.size()),
        _free_edges.data(),
        _free_edges.data() + _free_edges.size(),
    };
}

} // namespace clothoid
