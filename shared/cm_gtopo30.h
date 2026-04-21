// shared/cm_gtopo30.h
//

#pragma once

#include "cm_vector.h"

#include <cstddef>
#include <cstdint>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// Container for USGS 30 ARC-second Global Elevation Data, GTOPO30
//
// https://osdata.gdex.ucar.edu/web/datasets/d758000/docs/readme.txt

//------------------------------------------------------------------------------
class gtopo30
{
public:
    gtopo30();

    float height(vec3 v) const;

private:
    struct tile {
        std::size_t nrows; //!< number of rows in the tile
        std::size_t ncols; //!< number of columns in the tile

        double ulxmap; //!< longitude of the center of the upper-left pixel (decimal degrees)
        double ulymap; //!< latitude of the center of the upper-left pixel (decimal degrees)
        double xdim; //!< x dimension of a pixel in geographic units (decimal degrees)
        double ydim; //!< y dimension of a pixel in geographic units (decimal degrees)

        //! Tile data, arranged row-major, left to right, top to bottom
        std::vector<int16_t> data;
    };

    tile _tiles[33];

private:
    void load_tile(int lng, int lat, tile& tile) const;
};
