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
                _faces.push_back({{0, 1, 2}, {-1, -1, -1}});
             } else {
                _faces.push_back({{0, 2, 1}, {-1, -1, -1}});
             }
         }
        return true;
    }

    // general case

    // find the face that contains v
    for (int f0 = 0, num_faces = narrow_cast<int>(_faces.size()); f0 < num_faces; ++f0) {
        int v0 = _faces[f0].vert[0];
        int v1 = _faces[f0].vert[1];
        int v2 = _faces[f0].vert[2];
        vec2 p0 = _verts[v0];
        vec2 p1 = _verts[v1];
        vec2 p2 = _verts[v2];

        // calculate barycentric coordinates of v within [p0,p1,p2]
        float det = (p0 - p2).cross(p2 - p1);
        float x = (v - p2).cross(p2 - p1) / det;
        float y = (v - p2).cross(p0 - p2) / det;

        if (x < 0.f || y < 0.f || x + y > 1.f) {
            continue;
        }

        /*
            split face

                  v0
                 /| \
             f2 / |  \ f0
               /  v3  \
              /  /  \  \
             v2--------v1
                  f1
                                 v0 > v1 > v3
             v0 > v1 > v2   =>   v1 > v2 > v3
                                 v2 > v0 > v3
        */
        int v3 = narrow_cast<int>(_verts.size());
        _verts.push_back(v);

        int o0 = _faces[f0].opposite[0];
        int o1 = _faces[f0].opposite[1];
        int o2 = _faces[f0].opposite[2];

        int f1 = narrow_cast<int>(_faces.size() + 0);
        int f2 = narrow_cast<int>(_faces.size() + 1);
        _faces.resize(f2 + 1);

        _faces[f0] = {{v0, v1, v3}, {o0, f1, f2}};
        _faces[f1] = {{v1, v2, v3}, {o1, f2, f0}};
        _faces[f2] = {{v2, v0, v3}, {o2, f0, f1}};

        for (int ii = 0; ii < 3; ++ii) {
            if (o1 != -1 && _faces[o1].opposite[ii] == f0) {
                _faces[o1].opposite[ii] = f1;
            }
            if (o2 != -1 && _faces[o2].opposite[ii] == f0) {
                _faces[o2].opposite[ii] = f2;
            }
        }

        // TODO: update edge-pairs, not faces
        update(f0);
        update(f1);
        update(f2);
        return true;
    }

    // find the nearest edge to v
    float min_dist = INFINITY;
    int min_face = -1;
    int min_edge = -1;
    for (int face_index = 0, num_faces = narrow_cast<int>(_faces.size()); face_index < num_faces; ++face_index) {
        for (int edge_index = 0; edge_index < 3; ++edge_index) {
            // skip interior edges
            if (_faces[face_index].opposite[edge_index] != -1) {
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

            vec2 v0 = _verts[_faces[face_index].vert[(edge_index + 1) % 3]];
            vec2 v1 = _verts[_faces[face_index].vert[(edge_index + 0)    ]];

            // ignore back-facing edges
            if ((v - v0).cross(v1 - v0) < 0.f) {
                continue;
            }

            // distance to point on line [v0,v1] nearest to v
            vec2 p = nearest_point(v0, v1, v);
            float dsqr = (v - p).length_sqr();
            if (dsqr < min_dist) {
                min_dist = dsqr;
                min_face = face_index;
                min_edge = edge_index;
            }
        }
    }

    assert(min_face != -1);
    assert(min_edge != -1);

    // create new triangle with min_edge and v
    {
        int v0 = _faces[min_face].vert[(min_edge + 1) % 3];
        int v1 = _faces[min_face].vert[(min_edge + 0)    ];

        int new_vert = narrow_cast<int>(_verts.size());
        int new_face = narrow_cast<int>(_faces.size());

        _verts.push_back(v);
        _faces.push_back({{v0, v1, new_vert}, {min_face, -1, -1}});
        _faces[min_face].opposite[min_edge] = new_face;
        update(new_face);
        return true;
    }
}

//------------------------------------------------------------------------------
void delaunay::update(int face)
{
    std::queue<int> queue({face});
    while (queue.size()) {
        int face_index = queue.front();
        queue.pop();

        for (int edge_index = 0; edge_index < 3; ++edge_index) {

            // skip exterior edges
            int opposite_face = _faces[face_index].opposite[edge_index];
            if (opposite_face == -1) {
                continue;
            }

            /*
                v1-------v2
                 | \  f1 |
                 |   \   |
                 | f2  \ |
                v3-------v0
            */
            int v0 = _faces[face_index].vert[(edge_index + 0)    ];
            int v1 = _faces[face_index].vert[(edge_index + 1) % 3];
            int v2 = _faces[face_index].vert[(edge_index + 2) % 3];
            int v3 = -1;

            vec2 a = _verts[v1];
            vec2 b = _verts[v2];
            vec2 c = _verts[v0];
            vec2 d;

            int opposite_edge = 0;
            for (; opposite_edge < 3; ++opposite_edge) {
                if (_faces[opposite_face].vert[opposite_edge] == v1) {
                    v3 = _faces[opposite_face].vert[(opposite_edge + 2) % 3];
                    d = _verts[v3];
                    break;
                }
            }

            assert(v3 != -1); // this indicates internal inconsistency (bad!)

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

                        f12
                    v1-------v2          v1-------v2
                     | \  f1 |            | f2  / |
                     |   \   |     =>     |   /   |
                     | f2  \ |            | /  f1 |
                    v3-------v0          v3-------v0
                        f03

                    v0 > v1 > v2   =>   v0 > v3 > v2   (face_index)
                    v1 > v0 > v3   =>   v1 > v2 > v3   (opposite_face)
            */

            _faces[face_index].vert[(edge_index + 1) % 3] = v3;
            _faces[opposite_face].vert[(opposite_edge + 1) % 3] = v2;

            // update adjacency
            int face12 = _faces[face_index].opposite[(edge_index + 1) % 3];
            int face03 = _faces[opposite_face].opposite[(opposite_edge + 1) % 3];
            for (int ii = 0; ii < 3; ++ii) {
                if (face12 != -1 && _faces[face12].opposite[ii] == face_index) {
                    assert(_faces[face12].vert[ii] == v2);
                    _faces[face12].opposite[ii] = opposite_face;
                }
                if (face03 != -1 && _faces[face03].opposite[ii] == opposite_face) {
                    assert(_faces[face03].vert[ii] == v3);
                    _faces[face03].opposite[ii] = face_index;
                }
            }
            _faces[face_index].opposite[(edge_index + 0)    ] = face03;
            _faces[face_index].opposite[(edge_index + 1) % 3] = opposite_face;
            _faces[opposite_face].opposite[(opposite_edge + 0)    ] = face12;
            _faces[opposite_face].opposite[(opposite_edge + 1) % 3] = face_index;

            // add neighboring faces to queue
            for (int ii = 1; ii < 3; ++ii) {
                if (_faces[face_index].opposite[(edge_index + ii) % 3] != -1) {
                    queue.push(_faces[face_index].opposite[(edge_index + ii) % 3]);
                }
                if (_faces[opposite_face].opposite[(opposite_edge + ii) % 3] != -1) {
                    queue.push(_faces[opposite_face].opposite[(opposite_edge + ii) % 3]);
                }
            }
        }
    }
}
