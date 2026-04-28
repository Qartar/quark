// shared/cm_gshhg.h
//

#pragma once

#include "cm_vector.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
// Global Self-consistent Hierarchical High-resolution Geography, GSHHG
//
// https://www.soest.hawaii.edu/pwessel/gshhg/

//------------------------------------------------------------------------------
class gshhg
{
public:
    enum class resolution {
        full,
        high,
        intermediate,
        low,
        crude,
    };

    struct poly {
        int start; //!< First vertex of polygon
        int count; //!< Number of vertices in polygon
        int flags;
    };

public:
    gshhg();

    bool load(resolution res);
    void clear();

    std::vector<poly> const& polygons() const { return _polygons; }
    std::vector<vec3f> const& vertices() const { return _vertices; }

protected:
    std::vector<poly> _polygons;
    std::vector<vec3f> _vertices;
};
