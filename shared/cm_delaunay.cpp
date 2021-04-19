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
    if (_verts.size() < 3) {
        _verts.push_back(v);
         if (_verts.size() == 3) {
             // clockwise winding
             if ((_verts[2] - _verts[0]).cross(_verts[1] - _verts[0]) > 0.f) {
                _edge_verts.push_back(0);
                _edge_verts.push_back(1);
                _edge_verts.push_back(2);
             } else {
                _edge_verts.push_back(0);
                _edge_verts.push_back(2);
                _edge_verts.push_back(1);
             }
            _edge_pairs.push_back(-1);
            _edge_pairs.push_back(-1);
            _edge_pairs.push_back(-1);
         }
        return true;
    }

    // general case

    // find the face that contains v
    for (int e0 = 0, num_edges = narrow_cast<int>(_edge_verts.size()); e0 < num_edges; e0 += 3) {
        if (split_face(v, e0)) {
            return true;
        }
    }

    // find the nearest edge to v
    float min_dist = INFINITY;
    int min_edge = -1;
    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; ++edge_index) {
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
        int v2 = narrow_cast<int>(_verts.size());

        _verts.push_back(v);
        _edge_verts.push_back(v0);
        _edge_verts.push_back(v1);
        _edge_verts.push_back(v2);

        _edge_pairs.push_back(min_edge);
        _edge_pairs.push_back(-1);
        _edge_pairs.push_back(-1);

        _edge_pairs[min_edge] = narrow_cast<int>(_edge_pairs.size() - 3);

        update_edges({min_edge});
        return true;
    }
}

//------------------------------------------------------------------------------
bool delaunay::split_face(vec2 v, int e0)
{
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
    if (x < 0.f || y < 0.f || x + y > det) {
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
    int v3 = narrow_cast<int>(_verts.size());
    _verts.push_back(v);

    /*int o0 = _edge_pairs[e0 + 0];*/
    int o1 = _edge_pairs[e0 + 1];
    int o2 = _edge_pairs[e0 + 2];

    _edge_verts.resize(_edge_verts.size() + 6);
    _edge_pairs.resize(_edge_pairs.size() + 6);

    int e3 = narrow_cast<int>(_edge_verts.size() - 6);

    /*_edge_verts[e0 + 0] = v0;*/
    /*_edge_verts[e0 + 1] = v1;*/
    _edge_verts[e0 + 2] = v3;

    _edge_verts[e3 + 0] = v1;
    _edge_verts[e3 + 1] = v2;
    _edge_verts[e3 + 2] = v3;

    _edge_verts[e3 + 3] = v2;
    _edge_verts[e3 + 4] = v0;
    _edge_verts[e3 + 5] = v3;

    /*_edge_pairs[e0 + 0] = o0;*/   // v0 -> v1
    _edge_pairs[e0 + 1] = e3 + 2;   // v1 -> v3
    _edge_pairs[e0 + 2] = e3 + 4;   // v3 -> v0

    _edge_pairs[e3 + 0] = o1;       // v1 -> v2
    _edge_pairs[e3 + 1] = e3 + 5;   // v2 -> v3
    _edge_pairs[e3 + 2] = e0 + 1;   // v3 -> v1

    _edge_pairs[e3 + 3] = o2;       // v2 -> v0
    _edge_pairs[e3 + 4] = e0 + 2;   // v0 -> v3
    _edge_pairs[e3 + 5] = e3 + 1;   // v3 -> v2

    if (o1 != -1) {
        _edge_pairs[o1] = e3 + 0;
    }
    if (o2 != -1) {
        _edge_pairs[o2] = e3 + 3;
    }

    // check new and original edges of split triangle
    update_edges({e0, e0 + 1, e3 + 0, e3 + 1, e3 + 3, e3 + 4});
    return true;
}

//------------------------------------------------------------------------------
int delaunay::intersect_point(vec2 point) const
{
    for (int e0 = 0, num_edges = narrow_cast<int>(_edge_verts.size()); e0 < num_edges; e0 += 3) {
        int v0 = _edge_verts[e0 + 0];
        int v1 = _edge_verts[e0 + 1];
        int v2 = _edge_verts[e0 + 2];
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
int delaunay::nearest_edge(vec2 point) const
{
    float best_dsqr = INFINITY;
    int best_edge = -1;

    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; ++edge_index) {
        vec2 v0 = _verts[_edge_verts[edge_offset<1>(edge_index)]];
        vec2 v1 = _verts[_edge_verts[edge_offset<0>(edge_index)]];

        // ignore back-facing edges
        if ((point - v0).cross(v1 - v0) >= 0.f) {
            // distance to point on line [v0,v1] nearest to v
            vec2 p = nearest_point_on_line(v0, v1, point);
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
