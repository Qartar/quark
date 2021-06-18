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
    int num_verts() const { return static_cast<int>(_verts.size() - _free_verts.size()); }
    int num_edges() const { return static_cast<int>(_edge_verts.size() - _free_faces.size() * 3); }
    int num_faces() const { return static_cast<int>(_edge_verts.size() / 3 - _free_faces.size()); }

    //! insert the given vertex into the triangulation
    bool insert_vertex(vec2 v);
    //! remove the given vertex from the triangulation, replacing the faces containing the vertex with new triangles
    bool remove_vertex(int vert_index);

    template<int N>
    int find_path(vec2 (&points)[N], vec2 start, vec2 end) const { return find_path(points, N, start, end); }
    int find_path(vec2* points, int max_points, vec2 start, vec2 end) const;

    template<int N>
    int line_of_sight(vec2 (&points)[N], vec2 start, vec2 end) const { return line_of_sight(points, N, start, end); }
    int line_of_sight(vec2* points, int max_points, vec2 start, vec2 end) const;

    //! return the previous edge starting from the same vertex as the given edge or -1 if no such edge exists
    int prev_vertex_edge(int edge_index) const { return _edge_pairs[edge_offset<2>(edge_index)]; }
    //! return the next edge starting from the same vertex as the given edge or -1 if no such edge exists
    int next_vertex_edge(int edge_index) const { return _edge_pairs[edge_index] == -1 ? -1 : edge_offset<1>(_edge_pairs[edge_index]); }

    //! return the previous boundary edge or -1 if the given edge is not a boundary edge
    int prev_boundary_edge(int edge_index) const;
    //! return the next boundary edge or -1 if the given edge is not a boundary edge
    int next_boundary_edge(int edge_index) const;

    //! split the face containing the given edge by inserting the given point into the triangulation
    bool split_face(vec2 point, int edge_index);

    //! return the first edge of the triangle containing the given point or -1 if no such triangle exists
    int intersect_point(vec2 point) const;
    //! return the nearest vertex to the given point or -1 if no such vertex exists
    int nearest_vertex(vec2 point) const;
    //! return the nearest edge to the given point or -1 if no such edge exists
    int nearest_edge(vec2 point) const;

    bool get_vertex(int vert_index, vec2& v0) const;
    bool get_edge(int edge_index, vec2& v0, vec2& v1) const;
    bool get_face(int edge_index, vec2& v0, vec2& v1, vec2& v2) const;

    //! returns true if the given vertex belongs to a boundary edge
    bool is_boundary_vertex(int vert_index) const;
    //! returns true if the given edge is a boundary edge
    bool is_boundary_edge(int edge_index) const { return _edge_pairs[edge_index] == -1; }
    //! returns true if the given face contains a boundary edge
    bool is_boundary_face(int edge_index) const;

    void refine();

    //! remove all free edges and vertices
    void shrink_to_fit();

protected:
    friend render::system;

    std::vector<vec2> _verts;
    std::vector<int> _edge_verts; //!< index of originating vertex of the edge
    std::vector<int> _edge_pairs; //!< index of corresponding edge in adjacent face

    std::vector<int> _free_verts; //!< free list of vertices
    std::vector<int> _free_faces; //!< free list of faces, i.e. three consecutive edges

protected:
    //! check delaunay condition starting at the given edge, flip adjacent
    //! faces and iterate over neighboring edges as necessary
    template<int N> void update_edges(int const (&edges)[N]) { return update_edges(edges, N); }
    void update_edges(int const* edges, int num_edges);

    template<int N> void refine(int const (&edges)[N]) { return refine(edges, N); }
    void refine(int const* edges, int num_edges);

    //! return the index of the Nth edge in the same face starting at edge_index
    template<int N> static constexpr int edge_offset(int edge_index) {
        static_assert(N >= 0);
        return (edge_index / 3) * 3 + (edge_index + N) % 3;
    }

    int alloc_vert(vec2 v);
    int alloc_face(int v0, int v1, int v2);

    void free_vert(int vert_index);
    void free_face(int edge_index);
};
