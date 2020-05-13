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
    delaunay();

    bool insert_vertex(vec2 v);

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

template<> inline constexpr int delaunay::edge_offset<0>(int edge_index) { return edge_index; }
