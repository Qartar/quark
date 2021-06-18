//  cm_delaunay.cpp
//

#include "cm_delaunay.h"
#include "cm_geometry.h"
#include "cm_matrix.h"
#include "cm_shared.h"

#include <queue>
#include <unordered_set>

//------------------------------------------------------------------------------
bool delaunay::insert_vertex(vec2 v)
{
    if (num_verts() < 3) {
        alloc_vert(v);
         if (num_verts() == 3) {
             shrink_to_fit();
             // clockwise winding
             if ((_verts[2] - _verts[0]).cross(_verts[1] - _verts[0]) > 0.f) {
                 alloc_face(0, 1, 2);
             } else {
                 alloc_face(0, 2, 1);
             }
         }
        return true;
    }

    // general case

    // find the face that contains v
#if 0
    for (int e0 = 0, num_edges = narrow_cast<int>(_edge_verts.size()); e0 < num_edges; e0 += 3) {
        if (split_face(v, e0)) {
            return true;
        }
    }
#else
    // only expand boundary edges
    if (intersect_point(v) != -1) {
        return false;
    }
#endif

    // find the nearest edge to v
    float min_dist = INFINITY;
    int min_edge = -1;
    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; ++edge_index) {
        if (_edge_verts[edge_index] == -1) {
            continue;
        }
        // skip interior edges
        if (_edge_pairs[edge_index] != -1) {
            continue;
        }

        vec2 v0 = _verts[_edge_verts[edge_offset<1>(edge_index)]];
        vec2 v1 = _verts[_edge_verts[edge_offset<0>(edge_index)]];

        // ignore back-facing edges
        if ((v - v0).cross(v1 - v0) < 0.f) {
            continue;
        }

        // distance to point on line [v0,v1] nearest to v
        vec2 p = nearest_point_on_line(v0, v1, v);
        float dsqr = (v - p).length_sqr();
        if (dsqr < min_dist) {
            min_dist = dsqr;
            min_edge = edge_index;
        }
    }

    assert(min_edge != -1);

    // create new triangle with min_edge and v
    {
        int v0 = _edge_verts[edge_offset<1>(min_edge)];
        int v1 = _edge_verts[edge_offset<0>(min_edge)];
        int v2 = alloc_vert(v);

        int edge_index = alloc_face(v0, v1, v2);
        _edge_pairs[edge_index] = min_edge;
        _edge_pairs[min_edge] = edge_index;

        update_edges({min_edge});
        refine();
        return true;
    }
}

//------------------------------------------------------------------------------
bool delaunay::remove_vertex(int vert_index)
{
    if (vert_index < 0 || vert_index >= _verts.size()) {
        return false;
    }

    // find the first edge containing the given vertex
    int edge_index = -1;
    for (int ei = 0, sz = narrow_cast<int>(_edge_verts.size()); ei < sz; ++ei) {
        if (_edge_verts[ei] == vert_index) {
            edge_index = ei;
            break;
        }
    }

    if (edge_index == -1) {
        return false;
    }

    // Collect list of vertices and edges around the
    // region surrounding the vertex to be removed.
    std::vector<int> verts;
    std::vector<int> edges;

    for (int e0 = edge_index; e0 != -1;) {
        int e1 = edge_offset<1>(e0);
        verts.push_back(_edge_verts[e1]);
        edges.push_back(_edge_pairs[e1]);
        // Disconnect the boundary edge from the removed face
        if (edges.back() != -1) {
            _edge_pairs[edges.back()] = -1;
        }

        free_face(e0 - e0 % 3);

        e0 = next_vertex_edge(e0);
        if (e0 == edge_index) {
            break;
        }
    }

    free_vert(vert_index);

    std::vector<int> faces;

    // Iteratively fill the removed region with new triangles
    while (verts.size() >= 3) {
        int v0 = verts[verts.size() - 3];
        int v1 = verts[verts.size() - 1];
        int v2 = verts[verts.size() - 2];

        // TODO: Prove that this check is sufficient to prevent invalid tessellations.
        // THIS IS NOT SUFFICIENT TO PREVENT INVALID TESSELLATIONS
        if (verts.size() >= 4) {
            vec2 p0 = _verts[v0];
            vec2 p1 = _verts[v1];
            vec2 p2 = _verts[v2];
            vec2 p3 = _verts[verts[verts.size() - 4]];

            // Check if either the current or subsequent set of vertices would
            // result in an inverted triangle. Checking both prevents situations
            // where an assignment forces the remaining faces to be inverted.
            float det012 = (p0 - p2).cross(p2 - p1);
            float det310 = (p3 - p0).cross(p3 - p1);
            if (det012 <= 0.f || det310 <= 0.f) {
                std::rotate(verts.begin(), verts.begin() + 1, verts.end());
                std::rotate(edges.begin(), edges.begin() + 1, edges.end());
                continue;
            }
        }

        faces.push_back(alloc_face(v0, v1, v2));

        // Update edges on the new triangle to correspond with boundary edges
        _edge_pairs[faces.back() + 0LL] = verts.size() == 3 ? edges[0] : -1;
        _edge_pairs[faces.back() + 1LL] = edges[edges.size() - 1];
        _edge_pairs[faces.back() + 2LL] = edges[edges.size() - 2];

        // Update edges on the boundary to correspond with the triangle edges
        for (int jj = 0; jj < 3; ++jj) {
            int e0 = _edge_pairs[faces.back() + jj];
            if (e0 != -1) {
                _edge_pairs[e0] = faces.back() + jj;
            }
        }

        // Update boundary region to include new triangle
        verts.pop_back();
        verts.back() = v1;
        edges.pop_back();
        edges.back() = faces.back();
    }

    update_edges(faces.data(), narrow_cast<int>(faces.size()));
    shrink_to_fit();
    return true;
}

//------------------------------------------------------------------------------
bool delaunay::split_face(vec2 v, int e0)
{
    e0 -= e0 % 3;
    int v0 = _edge_verts[e0 + 0];
    int v1 = _edge_verts[e0 + 1];
    int v2 = _edge_verts[e0 + 2];
    vec2 p0 = _verts[v0];
    vec2 p1 = _verts[v1];
    vec2 p2 = _verts[v2];

    // calculate barycentric coordinates of v within [p0,p1,p2]
    float det = (p0 - p2).cross(p2 - p1);
    float x = (v - p2).cross(p2 - p1);
    float y = (v - p2).cross(p0 - p2);

    assert(det >= 0.f);
    if (x <= 0.f || y <= 0.f || x + y >= det) {
        return false;
    }

    /*
        split face

                v0
                /| \
            e2 / |  \ e0
              /  v3  \
             /  /  \  \
            v2--------v1
                e1
                                v0 > v1 > v3
            v0 > v1 > v2   =>   v1 > v2 > v3
                                v2 > v0 > v3
    */
    int v3 = alloc_vert(v);

    /*int o0 = _edge_pairs[e0 + 0];*/
    int o1 = _edge_pairs[e0 + 1];
    int o2 = _edge_pairs[e0 + 2];

    /*_edge_verts[e0 + 0] = v0;*/
    /*_edge_verts[e0 + 1] = v1;*/
    _edge_verts[e0 + 2] = v3;

    int e3 = alloc_face(v1, v2, v3);
    int e6 = alloc_face(v2, v0, v3);

    /*_edge_pairs[e0 + 0] = o0;*/   // v0 -> v1
    _edge_pairs[e0 + 1] = e3 + 2;   // v1 -> v3
    _edge_pairs[e0 + 2] = e6 + 1;   // v3 -> v0

    _edge_pairs[e3 + 0] = o1;       // v1 -> v2
    _edge_pairs[e3 + 1] = e6 + 2;   // v2 -> v3
    _edge_pairs[e3 + 2] = e0 + 1;   // v3 -> v1

    _edge_pairs[e6 + 0] = o2;       // v2 -> v0
    _edge_pairs[e6 + 1] = e0 + 2;   // v0 -> v3
    _edge_pairs[e6 + 2] = e3 + 1;   // v3 -> v2

    if (o1 != -1) {
        _edge_pairs[o1] = e3 + 0;
    }
    if (o2 != -1) {
        _edge_pairs[o2] = e6 + 0;
    }

    // check new and original edges of split triangle
    update_edges({e0, e0 + 1, e3 + 0, e3 + 1, e6 + 0, e6 + 1});
    return true;
}

//------------------------------------------------------------------------------
int delaunay::intersect_point(vec2 point) const
{
    for (int e0 = 0, num_edges = narrow_cast<int>(_edge_verts.size()); e0 < num_edges; e0 += 3) {
        int v0 = _edge_verts[e0 + 0LL];
        int v1 = _edge_verts[e0 + 1LL];
        int v2 = _edge_verts[e0 + 2LL];
        if (v0 == -1 || v1 == -1 || v2 == -1) {
            assert(v0 == -1 && v1 == -1 && v2 == -1);
            continue;
        }
        vec2 p0 = _verts[v0];
        vec2 p1 = _verts[v1];
        vec2 p2 = _verts[v2];

        // calculate barycentric coordinates of v within [p0,p1,p2]
        float det = (p0 - p2).cross(p2 - p1);
        float x = (point - p2).cross(p2 - p1);
        float y = (point - p2).cross(p0 - p2);

        assert(det >= 0.f);
        if (x >= 0.f && y >= 0.f && x + y <= det) {
            return e0;
        }
    }

    return -1;
}

//------------------------------------------------------------------------------
int delaunay::nearest_vertex(vec2 point) const
{
    float best_dsqr = INFINITY;
    int best_vert = -1;

    for (int vert_index = 0, num_verts = narrow_cast<int>(_verts.size()); vert_index < num_verts; ++vert_index) {
        float dsqr = (_verts[vert_index] - point).length_sqr();
        if (dsqr < best_dsqr) {
            best_dsqr = dsqr;
            best_vert = vert_index;
        }
    }

    return best_vert;
}

//------------------------------------------------------------------------------
int delaunay::nearest_edge(vec2 point) const
{
    float best_dsqr = INFINITY;
    int best_edge = -1;

    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; ++edge_index) {
        int v0 = _edge_verts[edge_offset<1>(edge_index)];
        int v1 = _edge_verts[edge_offset<0>(edge_index)];
        if (v0 == -1 || v1 == -1) {
            assert(v0 == -1 && v1 == -1);
            continue;
        }
        vec2 p0 = _verts[v0];
        vec2 p1 = _verts[v1];

        // ignore back-facing edges
        if ((point - p0).cross(p1 - p0) >= 0.f) {
            // distance to point on line [v0,v1] nearest to v
            vec2 p = nearest_point_on_line(p0, p1, point);
            float dsqr = (point - p).length_sqr();
            if (dsqr < best_dsqr) {
                best_dsqr = dsqr;
                best_edge = edge_index;
            }
        }
    }

    return best_edge;
}

//------------------------------------------------------------------------------
bool delaunay::get_vertex(int vert_index, vec2& v0) const
{
    if (vert_index >= 0 && vert_index < _verts.size()) {
        v0 = _verts[vert_index];
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool delaunay::get_edge(int edge_index, vec2& v0, vec2& v1) const
{
    if (edge_index >= 0 && edge_index < _edge_verts.size()) {
        v0 = _verts[_edge_verts[edge_offset<1>(edge_index)]];
        v1 = _verts[_edge_verts[edge_offset<0>(edge_index)]];
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool delaunay::get_face(int edge_index, vec2& v0, vec2& v1, vec2& v2) const
{
    assert(edge_index == -1 || edge_index % 3 == 0);
    if (edge_index >= 0 && edge_index < _edge_verts.size() - 2) {
        v0 = _verts[_edge_verts[edge_index + 0LL]];
        v1 = _verts[_edge_verts[edge_index + 1LL]];
        v2 = _verts[_edge_verts[edge_index + 2LL]];
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool delaunay::is_boundary_vertex(int /*vert_index*/) const
{
    assert(false);
    return false;
}

//------------------------------------------------------------------------------
bool delaunay::is_boundary_face(int edge_index) const
{
    int e0 = edge_index - edge_index % 3;
    return (_edge_pairs[e0] == -1 || _edge_pairs[e0 + 1LL] == -1 || _edge_pairs[e0 + 2LL] == -1);
}

//------------------------------------------------------------------------------
void delaunay::update_edges(int const* edges, int num_edges)
{
    std::queue<int> queue;
    for (int ii = 0; ii < num_edges; ++ii) {
        queue.push(edges[ii]);
    }

    while (queue.size()) {
        int edge_index = queue.front();
        queue.pop();

        // this occurs when a face containing a boundary edge is flipped
        if (edge_index == -1) {
            continue;
        }

        // skip exterior edges
        int opposite_edge = _edge_pairs[edge_index];
        if (opposite_edge == -1) {
            continue;
        }

        /*
            a-----------b
            | \   e1    |
            | e3\     e2|
            |     \     |
            |e5     \e0 |
            |    e4   \ |
            d-----------c
        */

        int e0 = edge_offset<0>(edge_index);
        int e1 = edge_offset<1>(edge_index);
        int e2 = edge_offset<2>(edge_index);
        int e3 = edge_offset<0>(opposite_edge);
        int e4 = edge_offset<1>(opposite_edge);
        int e5 = edge_offset<2>(opposite_edge);

        vec2 a = _verts[_edge_verts[e1]];
        vec2 b = _verts[_edge_verts[e2]];
        vec2 c = _verts[_edge_verts[e0]];
        vec2 d = _verts[_edge_verts[e5]];

        // check delaunay condition
        mat3 m(vec3(a - d, length_sqr(a - d)),
               vec3(b - d, length_sqr(b - d)),
               vec3(c - d, length_sqr(c - d)));

        float det = dot(m[0], cross(m[1], m[2]));

        // i.e. det > 0 and !nan(det)
        if (!(det <= 0.f)) {
            continue;
        }

        /*
            flip triangle edges

                   o1                       o1
            v1-----------v2          v1-----------v2
             | \   e1    |            |     e3  / |
             | e3\     e2|            |e5     /e1 |
             |     \     |     =>     |     /     |
             |e5     \e0 |            | e4/     e2|
             |    e4   \ |            | /   e0    |
            v3-----------v0          v3-----------v0
                  o4                        o4

             v0 > v1 > v2      =>      v0 > v3 > v2
             v1 > v0 > v3      =>      v1 > v2 > v3
        */

        _edge_verts[e1] = _edge_verts[e5];
        _edge_verts[e4] = _edge_verts[e2];

        // update adjacency
        int o1 = _edge_pairs[e1];
        int o4 = _edge_pairs[e4];

        if (o1 != -1) {
            _edge_pairs[o1] = e3;
        }
        if (o4 != -1) {
            _edge_pairs[o4] = e0;
        }

        _edge_pairs[e0] = o4;
        _edge_pairs[e1] = e4;
        _edge_pairs[e3] = o1;
        _edge_pairs[e4] = e1;

        // add boundary edges to queue
        queue.push(_edge_pairs[e0]);
        queue.push(_edge_pairs[e2]);
        queue.push(_edge_pairs[e3]);
        queue.push(_edge_pairs[e5]);
    }
}

//------------------------------------------------------------------------------
void delaunay::refine()
{
    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; edge_index += 3) {
        int e0 = edge_offset<0>(edge_index);
        int e1 = edge_offset<1>(edge_index);
        int e2 = edge_offset<2>(edge_index);

        if (_edge_verts[e0] == -1 || _edge_verts[e1] == -1 || _edge_verts[e2] == -1) {
            assert(_edge_verts[e0] == -1 && _edge_verts[e1] == -1 && _edge_verts[e2] == -1);
            continue;
        }
#if 0
        vec2 v0 = _verts[_edge_verts[e1]];
        vec2 v1 = _verts[_edge_verts[e2]];
        vec2 v2 = _verts[_edge_verts[e0]];

        float asqr = (v1 - v0).length_sqr();
        float bsqr = (v2 - v1).length_sqr();
        float csqr = (v0 - v2).length_sqr();

        float den = 0.5f / mat2(v1 - v0, v2 - v0).determinant();
        float rsqr = asqr * bsqr * csqr * den * den;

        // check triangle quality
        if (rsqr < std::min({asqr, bsqr, csqr})) {
            continue;
        }

        mat2 xy = mat2(v1 - v0, v2 - v0).transpose();
        vec2 z = vec2((v1 - v0).length_sqr(), (v2 - v0).length_sqr());
            // = (v1 - v0) * (v1 - v0), (v2 - v0) * (v2 - v0)

        float mx = v0.x + mat2(xy[1], z).determinant() * den;
        float my = v0.y + mat2(xy[0], z).determinant() * den;
#else
        vec2 a = _verts[_edge_verts[e1]];
        vec2 b = _verts[_edge_verts[e2]];
        vec2 c = _verts[_edge_verts[e0]];

        float xba = b.x - a.x;
        float yba = b.y - a.y;
        float xca = c.x - a.x;
        float yca = c.y - a.y;

        float balength = xba * xba + yba * yba;
        float calength = xca * xca + yca * yca;
        float cblength = (c - b).length_sqr();

        float denominator = 0.5f / (xba * yca - yba * xca);

        float radius = balength * calength * cblength * square(denominator);

        // check triangle quality
        if (radius < std::min({balength, calength, cblength})) {
            continue;
        }

        float mx = a.x + (yca * balength - yba * calength) * denominator;
        float my = a.y + (xba * calength - xca * balength) * denominator;
#endif

        int idx = intersect_point(vec2(mx, my));
        if (idx != -1) {
            split_face(vec2(mx, my), idx);
        }
    }
}

//------------------------------------------------------------------------------
void delaunay::refine(int const* edges, int num_edges)
{
    std::queue<int> queue;
    for (int ii = 0; ii < num_edges; ++ii) {
        queue.push(edges[ii]);
    }

    while (queue.size()) {
        int edge_index = queue.front();
        queue.pop();

        int e0 = edge_offset<0>(edge_index);
        int e1 = edge_offset<1>(edge_index);
        int e2 = edge_offset<2>(edge_index);
        vec2 v0 = _verts[_edge_verts[e1]];
        vec2 v1 = _verts[_edge_verts[e2]];
        vec2 v2 = _verts[_edge_verts[e0]];

        float asqr = (v1 - v0).length_sqr();
        float bsqr = (v2 - v1).length_sqr();
        float csqr = (v0 - v2).length_sqr();

        float den = 0.5f / mat2(v1 - v0, v2 - v0).determinant();
        float rsqr = asqr * bsqr * csqr * den * den;

        // check triangle quality
        if (rsqr < std::min({asqr, bsqr, csqr})) {
            continue;
        }

        mat2 xy = mat2(v1 - v0, v2 - v0).transpose();
        vec2 z = vec2((v1 - v0).length_sqr(), (v2 - v0).length_sqr());
            // = (v1 - v0) * (v1 - v0), (v2 - v0) * (v2 - v0)

        float mx = v0.x + mat2(xy[1], z).determinant() * den;
        float my = v0.y + mat2(xy[0], z).determinant() * den;

        int adj[3] = { _edge_pairs[e0], _edge_pairs[e1], _edge_pairs[e2] };
        for (int ii = 0; ii < countof(adj); ++ii) {
            if (adj[ii] != -1 && split_face(vec2(mx, my), adj[ii])) {
                queue.push(adj[ii]);
            }
        }
    }
}

//------------------------------------------------------------------------------
int delaunay::find_path(vec2* points, int max_points, vec2 start, vec2 end) const
{
    // find the faces containing start and end points
    int best_start_face = intersect_point(start);
    int best_end_face = intersect_point(end);

    // if start or end are not contained in the triangulation
    // then find the edge nearest to both start and end points
    int best_start_edge = (best_start_face == -1) ? nearest_edge(start) : -1;
    int best_end_edge = (best_end_face == -1) ? nearest_edge(end) : -1;

    // shouldn't be possible but not possible to continue in this state
    if ((best_start_face == -1 && best_start_edge == -1) || (best_end_face == -1 && best_end_edge == -1)) {
        assert(false);
        return -1;
    }

    // if start and end are in the same face
    if (best_start_face != -1 && best_start_face == best_end_face) {
        return 0;
    }

    struct search_state {
        vec2 position;
        float distance;
        float heuristic;
        std::size_t previous;
        int edge_index;
        int depth;
    };

    std::vector<search_state> search;

    struct search_comparator {
        decltype(search) const& _search;
        bool operator()(std::size_t lhs, std::size_t rhs) const {
            float d1 = _search[lhs].distance + _search[lhs].heuristic;
            float d2 = _search[rhs].distance + _search[rhs].heuristic;
            return d1 > d2;
        }
    };

    std::priority_queue<std::size_t, std::vector<std::size_t>, search_comparator> queue{{search}};
    std::unordered_set<int> closed_set;

    if (best_start_face == -1) {
        vec2 v0 = _verts[_edge_verts[edge_offset<1>(best_start_edge)]];
        vec2 v1 = _verts[_edge_verts[edge_offset<0>(best_start_edge)]];
        vec2 best_position = nearest_point_on_line(v0, v1, start);
        float best_start_dist = (best_position - start).length();
        float best_heuristic = (start - nearest_point_on_line(v0, v1, end)).length();

        search.push_back({best_position, best_start_dist, best_heuristic, SIZE_MAX, best_start_edge, 1});
        queue.push(search.size() - 1);
    } else {
        int e[3] = {
            edge_offset<0>(best_start_face),
            edge_offset<1>(best_start_face),
            edge_offset<2>(best_start_face),
        };
        vec2 v[3] = {
            _verts[_edge_verts[e[0]]],
            _verts[_edge_verts[e[1]]],
            _verts[_edge_verts[e[2]]],
        };

        for (int ii = 0; ii < 3; ++ii) {
            if (_edge_pairs[e[ii]] != -1) {
                // get midpoint of the edge
                vec2 p = (1.f / 2.f) * (v[ii] + v[(ii + 1) % 3]);
                search.push_back({p, length(p - start), length(p - end), SIZE_MAX, _edge_pairs[e[ii]], 1});
                queue.push(search.size() - 1);
            }
        }
    }

    while (queue.size()) {
        std::size_t idx = queue.top(); queue.pop();
        if (closed_set.find(search[idx].edge_index) != closed_set.cend()) {
            continue;
        }
        closed_set.insert(search[idx].edge_index);

        // search succeeded if edge index is in the same face or edge as the end point
        if (((search[idx].edge_index / 3) * 3 == best_end_face)
            || (best_end_face == -1 && (search[idx].edge_index / 3 == best_end_edge / 3))) {
            // copy path into buffer
            for (std::size_t ii = idx; ii != SIZE_MAX; ii = search[ii].previous) {
                points[search[ii].depth - 1] = search[ii].position;
            }
            return search[idx].depth;
        }

        // search failed
        if (search[idx].depth >= max_points) {
            return -1;
        }

        // iterate
        for (int edge_index = edge_offset<1>(search[idx].edge_index)
            ; edge_index != search[idx].edge_index
            ; edge_index = edge_offset<1>(edge_index)) {
            int opposite_edge = _edge_pairs[edge_index];
            // skip exterior edges
            if (opposite_edge == -1) {
                continue;
            }

            // get midpoint of next edge
            int v0 = _edge_verts[edge_offset<0>(opposite_edge)];
            int v1 = _edge_verts[edge_offset<1>(opposite_edge)];
            vec2 b = (1.f / 2.f) * (_verts[v0] + _verts[v1]);

            // add to search state
            search.push_back({
                b,
                search[idx].distance + length(b - search[idx].position),
                length(b - end),
                idx,
                opposite_edge,
                search[idx].depth + 1,
            });
            queue.push(search.size() - 1);
        }
    }

    // search failed
    return -1;
}

//------------------------------------------------------------------------------
int delaunay::line_of_sight(vec2* points, int max_points, vec2 start, vec2 end) const
{
    // find the nearest edge to v
    float min_dist = INFINITY;
    int min_edge = -1;
    vec2 min_point = vec2_zero;
    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; ++edge_index) {
        if (_edge_verts[edge_index] == -1) {
            continue;
        }
        vec2 v0 = _verts[_edge_verts[edge_offset<1>(edge_index)]];
        vec2 v1 = _verts[_edge_verts[edge_offset<0>(edge_index)]];

        // ignore back-facing edges
        if ((start - v0).cross(v1 - v0) < 0.f) {
            continue;
        }

        vec2 p;
        if (intersect_lines(v0, v1, start, end, p)) {
            float d = (p - start).dot(end - start);
            if (d >= 0.f && d < min_dist) {
                min_dist = d;
                min_edge = edge_index;
                min_point = p;
            }
        }
    }

    if (min_edge == -1) {
        return -1;
    }

    int num_points = 0;
    points[num_points++] = min_point;

    for (int edge_index = min_edge;;) {
        if (num_points == max_points) {
            return -1;
        }

        // find the boundary edge where the ray enters the triangulation
        if (edge_index == -1) {
            float best_d = INFINITY;
            int best_edge = -1;
            for (int ii = 0, num_edges = narrow_cast<int>(_edge_verts.size()); ii < num_edges; ++ii) {
                if (_edge_verts[ii] == -1) {
                    continue;
                }
                // skip interior edges
                if (_edge_pairs[ii] != -1) {
                    continue;
                }

                vec2 v0 = _verts[_edge_verts[edge_offset<1>(ii)]];
                vec2 v1 = _verts[_edge_verts[edge_offset<0>(ii)]];

                // ignore back-facing edges
                if ((start - v0).cross(v1 - v0) < 0.f) {
                    continue;
                }

                vec2 p;
                if (intersect_lines(v0, v1, start, end, p)) {
                    float d = (p - points[num_points - 1]).dot(end - start);
                    if (d > 0.f && d < best_d) {
                        best_d = d;
                        best_edge = ii;
                        points[num_points] = p;
                    }
                }
            }

            if (best_edge == -1) {
                return num_points;
            }

            edge_index = best_edge;
            ++num_points;

        // find the next edge in the current face along the ray direction
        } else {
            vec2 v0 = _verts[_edge_verts[edge_offset<0>(edge_index)]];
            vec2 v1 = _verts[_edge_verts[edge_offset<1>(edge_index)]];
            vec2 v2 = _verts[_edge_verts[edge_offset<2>(edge_index)]];

            if (intersect_lines(v1, v2, start, end, points[num_points])) {
                ++num_points;
                edge_index = _edge_pairs[edge_offset<1>(edge_index)];
            } else if (intersect_lines(v2, v0, start, end, points[num_points])) {
                ++num_points;
                edge_index = _edge_pairs[edge_offset<2>(edge_index)];
            } else {
                return num_points;
            }
        }
    }
}

//------------------------------------------------------------------------------
int delaunay::prev_boundary_edge(int edge_index) const
{
    int pivot_index = edge_offset<2>(edge_index);
    while (true) {
        int prev_index = _edge_pairs[pivot_index];
        // prev_index and edge_index start at the same vertex
        assert(prev_index == -1 || _edge_verts[prev_index] == _edge_verts[edge_index]);
        if (prev_index == -1) {
            return pivot_index;
        } else if (prev_index == edge_index) {
            // the given edge is not a boundary edge
            return -1;
        } else {
            pivot_index = edge_offset<2>(prev_index);
        }
    }
}

//------------------------------------------------------------------------------
int delaunay::next_boundary_edge(int edge_index) const
{
    int pivot_index = edge_offset<1>(edge_index);
    while (true) {
        int next_index = _edge_pairs[pivot_index];
        // edge_index and next_index end at the same vertex
        assert(next_index == -1 || _edge_verts[edge_offset<1>(edge_index)] == _edge_verts[edge_offset<1>(next_index)]);
        if (next_index == -1) {
            return pivot_index;
        } else if (next_index == edge_index) {
            // the given edge is not a boundary edge
            return -1;
        } else {
            pivot_index = edge_offset<1>(next_index);
        }
    }
}

//------------------------------------------------------------------------------
int delaunay::alloc_vert(vec2 v)
{
    if (!_free_verts.size()) {
        _verts.push_back(v);
        return narrow_cast<int>(_verts.size() - 1);
    } else {
        int vert_index = _free_verts.back();
        _free_verts.pop_back();
        _verts[vert_index] = v;
        return vert_index;
    }
}

//------------------------------------------------------------------------------
int delaunay::alloc_face(int v0, int v1, int v2)
{
    int edge_index;
    if (!_free_faces.size()) {
        assert(_edge_verts.size() == _edge_pairs.size());
        edge_index = narrow_cast<int>(_edge_verts.size());
        _edge_verts.resize(_edge_verts.size() + 3);
        _edge_pairs.resize(_edge_pairs.size() + 3);
    } else {
        edge_index = _free_faces.back();
        _free_faces.pop_back();
    }

    _edge_verts[edge_index + 0LL] = v0;
    _edge_verts[edge_index + 1LL] = v1;
    _edge_verts[edge_index + 2LL] = v2;

    _edge_pairs[edge_index + 0LL] = -1;
    _edge_pairs[edge_index + 1LL] = -1;
    _edge_pairs[edge_index + 2LL] = -1;

    return edge_index;
}

//------------------------------------------------------------------------------
void delaunay::free_vert(int vert_index)
{
    assert(vert_index < _verts.size());
    _free_verts.push_back(vert_index);
}

//------------------------------------------------------------------------------
void delaunay::free_face(int edge_index)
{
    assert(edge_index % 3 == 0);
    assert(edge_index < _edge_pairs.size() - 2);
    _free_faces.push_back(edge_index);
    _edge_verts[edge_index + 0LL] = -1;
    _edge_verts[edge_index + 1LL] = -1;
    _edge_verts[edge_index + 2LL] = -1;
}

//------------------------------------------------------------------------------
void delaunay::shrink_to_fit()
{
    // map from old index to new index
    std::vector<int> vert_map, edge_map;
    vert_map.resize(_verts.size());
    edge_map.resize(_edge_verts.size());

    // Build mapping for vertices.
    std::sort(_free_verts.begin(), _free_verts.end());
    for (int ii = 0, jj = 0, kk = 0, sz = narrow_cast<int>(_verts.size()); ii < sz; ++ii) {
        if (jj >= _free_verts.size() || ii < _free_verts[jj]) {
            vert_map[ii] = kk++;
            if (kk < ii) {
                _verts[kk] = _verts[ii];
            }
        } else {
            jj++;
        }
    }

    // Build mapping for edges. This needs to be done in two passes since edges
    // can refer to higher-indexed edges which won't yet be mapped on the first pass.
    std::sort(_free_faces.begin(), _free_faces.end());
    for (int ii = 0, jj = 0, kk = 0, sz = narrow_cast<int>(_edge_verts.size()); ii < sz; ii += 3) {
        if (jj >= _free_faces.size() || ii < _free_faces[jj]) {
            edge_map[ii + 0LL] = kk++;
            edge_map[ii + 1LL] = kk++;
            edge_map[ii + 2LL] = kk++;
        } else {
            jj++;
        }
    }

    // re-index verts and edges
    for (int ii = 0, jj = 0, kk = 0, sz = narrow_cast<int>(_edge_verts.size()); ii < sz; ii += 3) {
        if (jj >= _free_faces.size() || ii < _free_faces[jj]) {
            for (std::size_t ll = 0; ll < 3; ++kk, ++ll) {
                _edge_verts[kk] = vert_map[_edge_verts[ii + ll]];
                if (_edge_pairs[ii + ll] != -1) {
                    _edge_pairs[kk] = edge_map[_edge_pairs[ii + ll]];
                } else {
                    _edge_pairs[kk] = -1;
                }
            }
        } else {
            jj++;
        }
    }

    // Shrink arrays to fit
    _verts.resize(_verts.size() - _free_verts.size());
    _edge_verts.resize(_edge_verts.size() - _free_faces.size() * 3);
    _edge_pairs.resize(_edge_pairs.size() - _free_faces.size() * 3);
    _free_verts.clear();
    _free_faces.clear();
}
