//  cm_delaunay.cpp
//

#include "cm_delaunay.h"
#include "cm_matrix.h"
#include "cm_shared.h"

#include <queue>

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
            continue;
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

    // find the nearest edge to v
    float min_dist = INFINITY;
    int min_edge = -1;
    for (int edge_index = 0, num_edges = narrow_cast<int>(_edge_verts.size()); edge_index < num_edges; ++edge_index) {
        // skip interior edges
        if (_edge_pairs[edge_index] != -1) {
            continue;
        }

        // return point on line [a,b] nearest to c
        auto const nearest_point = [](vec2 a, vec2 b, vec2 c) {
            vec2 v = b - a;
            float num = (c - a).dot(v);
            float den = v.dot(v);
            if (num >= den) {
                return b;
            } else if (num < 0.f) {
                return a;
            } else {
                float t = num / den;
                float s = 1.f - t;
                return a * s + b * t;
            }
        };

        vec2 v0 = _verts[_edge_verts[edge_offset<1>(edge_index)]];
        vec2 v1 = _verts[_edge_verts[edge_offset<0>(edge_index)]];

        // ignore back-facing edges
        if ((v - v0).cross(v1 - v0) < 0.f) {
            continue;
        }

        // distance to point on line [v0,v1] nearest to v
        vec2 p = nearest_point(v0, v1, v);
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
