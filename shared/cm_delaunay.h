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

protected:
    friend render::system;

    struct edge {
        int vert;
        int face;
    };

    struct face {
        int vert[3];
        int opposite[3];
    };

    std::vector<vec2> _verts;
    std::vector<face> _faces; //!< TODO: use edges[nf*3] instead of faces[nf]

protected:
    //! check delaunay condition starting at the given face, flip adjacent
    //! faces and iterate over neighboring faces as necessary
    void update(int face);
};
