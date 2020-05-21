//  cm_delaunay.h
//

#pragma once

#include "cm_vector.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace render { class system; }

class delaunay
{
public:
    bool insert_vertex(vec2 v);

    template<int N>
    int find_path(vec2 (&points)[N], vec2 start, vec2 end) const { return find_path(points, N, start, end); }
    int find_path(vec2* points, int max_points, vec2 start, vec2 end) const;

    template<int N>
    int line_of_sight(vec2 (&points)[N], vec2 start, vec2 end) const { return line_of_sight(points, N, start, end); }
    int line_of_sight(vec2* points, int max_points, vec2 start, vec2 end) const;

    //! return the previous edge starting from the same vertex as the given edge or -1 if no such edge exists
    int prev_vertex_edge(int edge_index) const { return _edge_pairs[edge_offset<2>(edge_index)]; }
    //! return the next edge starting from the same vertex as the given edge or -1 if no such edge exists
    int next_vertex_edge(int edge_index) const { return _edge_pairs[edge_offset<1>(edge_index)]; }

    //! return the previous boundary edge or -1 if the given edge is not a boundary edge
    int prev_boundary_edge(int edge_index) const;
    //! return the next boundary edge or -1 if the given edge is not a boundary edge
    int next_boundary_edge(int edge_index) const;

protected:
    friend render::system;

    std::vector<vec2> _verts;
    std::vector<int> _edge_verts; //!< index of originating vertex of the edge
    std::vector<int> _edge_pairs; //!< index of corresponding edge in adjacent face

protected:
    //! check delaunay condition starting at the given edge, flip adjacent
    //! faces and iterate over neighboring edges as necessary
    template<int N> void update_edges(int const (&edges)[N]) { return update_edges(edges, N); }
    void update_edges(int const* edges, int num_edges);

    //! return the index of the Nth edge in the same face starting at edge_index
    template<int N> static constexpr int edge_offset(int edge_index) {
        static_assert(N >= 0);
        return (edge_index / 3) * 3 + (edge_index + N) % 3;
    }
};
