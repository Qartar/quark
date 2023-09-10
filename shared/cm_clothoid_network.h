// cm_clothoid_network.h
//

#pragma once

#include "cm_clothoid.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace clothoid {

//------------------------------------------------------------------------------
class network
{
public:
    using node_index = int;
    using edge_index = int;

    static constexpr node_index invalid_node = -1;
    static constexpr edge_index invalid_edge = -1;

    static constexpr float epsilon = 1e-3f;
    static constexpr float epsilon_sqr = 1e-6f;

    struct node {
        vec2 position; //!< start position of this node
        edge_index first_edge; //!< first edge starting at this node
    };

    struct edge {
        node_index end_node; //!< node at the end of the segment
        edge_index next_edge; //!< next edge at the start node
    };

    struct node_iterator {
        node_index node_begin;
        node_index node_end;
        node_index const* free_begin;
        node_index const* free_end;

        node_iterator& operator++() {
            ++node_begin;
            while(node_begin < node_end && free_begin < free_end) {
                if (*free_begin < node_begin) {
                    ++free_begin;
                } else if (*free_begin == node_begin) {
                    ++node_begin;
                    ++free_begin;
                } else {
                    break;
                }
            }
            return *this;
        }
        node_index operator*() const {
            return node_begin;
        }
        friend bool operator==(node_iterator const& lhs, node_iterator const& rhs) {
            return lhs.node_begin == rhs.node_begin;
        }
        friend bool operator!=(node_iterator const& lhs, node_iterator const& rhs) {
            return lhs.node_begin != rhs.node_begin;
        }
    };

    struct edge_iterator {
        edge_index edge_begin;
        edge_index edge_end;
        edge_index const* free_begin;
        edge_index const* free_end;

        edge_iterator& operator++() {
            ++edge_begin;
            while(edge_begin < edge_end && free_begin < free_end) {
                if (*free_begin < edge_begin) {
                    ++free_begin;
                } else if (*free_begin == edge_begin) {
                    edge_begin += 2; // edges are always allocated in pairs
                    ++free_begin;
                } else {
                    break;
                }
            }
            return *this;
        }
        edge_index operator*() const {
            return edge_begin;
        }
        friend bool operator==(edge_iterator const& lhs, edge_iterator const& rhs) {
            return lhs.edge_begin == rhs.edge_begin;
        }
        friend bool operator!=(edge_iterator const& lhs, edge_iterator const& rhs) {
            return lhs.edge_begin != rhs.edge_begin;
        }
    };

    struct node_range {
        node_index node_begin;
        node_index node_end;
        node_index const* free_begin;
        node_index const* free_end;

        node_iterator begin() {
            return ++node_iterator{node_begin - 1, node_end, free_begin, free_end};
        }
        node_iterator end() {
            return node_iterator{node_end, node_end, free_end, free_end};
        }
    };

    struct edge_range {
        edge_index edge_begin;
        edge_index edge_end;
        edge_index const* free_begin;
        edge_index const* free_end;

        edge_iterator begin() {
            return ++edge_iterator{edge_begin - 1, edge_end, free_begin, free_end};
        }
        edge_iterator end() {
            return edge_iterator{edge_end, edge_end, free_end, free_end};
        }
    };

public:
    edge_index insert_edge(segment s);
    void remove_edge(edge_index e);

    //! Insert a node along an existing edge by splitting it at the given position
    node_index insert_node(vec2 position);
    //! Remove the given node by merging connected edges if possible
    bool merge_node(node_index n);
    //! Remove the given node and all connected edges
    void remove_node(node_index n);

    //! Return the next edge starting from the same node as the given edge
    edge_index next_edge(edge_index e) const;

    //! Return the starting node of the given edge
    node_index start_node(edge_index e) const;
    //! Return the ending node of the given edge
    node_index end_node(edge_index e) const;
    //! Return the direction of the given edge
    vec2 edge_direction(edge_index e) const;

    node_range nodes() const;
    edge_range edges() const;

    node const& get_node(node_index n) const { return _nodes[n]; }
    edge const& get_edge(edge_index e) const { return _edges[e]; }
    segment const& get_segment(edge_index e) const { return _segments[e]; }

protected:
    //! Insert an existing edge into an existing node
    void node_insert_edge(node_index n, edge_index e);
    //!
    void node_remove_edge(node_index n, edge_index e);

    node_index alloc_node();
    void free_node(node_index n);

    //! Allocate a pair of edges
    edge_index alloc_edges();
    //! Free a pair of edges starting at index `e`
    void free_edges(edge_index e);

private:
    std::vector<node> _nodes;
    std::vector<edge> _edges;
    std::vector<segment> _segments;

    std::vector<node_index> _free_nodes;
    std::vector<edge_index> _free_edges;
};

} // namespace clothoid
