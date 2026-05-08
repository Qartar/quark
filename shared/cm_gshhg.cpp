// shared/cm_gshhg.cpp
//

#include "cm_gshhg.h"
#include "cm_filesystem.h"
#include "cm_shared.h"

#include <cstdint>
#include <vector>

//------------------------------------------------------------------------------
struct GSHHG {  /* Global Self-consistent Hierarchical High-resolution Shorelines */
        int id;         /* Unique polygon id number, starting at 0 */
        int n;          /* Number of points in this polygon */
        int flag;       /* = level + version << 8 + greenwich << 16 + source << 24 + river << 25 */
        /* flag contains 5 items, as follows:
         * low byte:    level = flag & 255: Values: 1 land, 2 lake, 3 island_in_lake, 4 pond_in_island_in_lake
         * 2nd byte:    version = (flag >> 8) & 255: Values: Should be 12 for GSHHG release 12 (i.e., version 2.2)
         * 3rd byte:    greenwich = (flag >> 16) & 1: Values: Greenwich is 1 if Greenwich is crossed
         * 4th byte:    source = (flag >> 24) & 1: Values: 0 = CIA WDBII, 1 = WVS
         * 4th byte:    river = (flag >> 25) & 1: Values: 0 = not set, 1 = river-lake and level = 2
         */
        int west, east, south, north;   /* min/max extent in micro-degrees */
        int area;       /* Area of polygon in 1/10 km^2 */
        int area_full;  /* Area of original full-resolution polygon in 1/10 km^2 */
        int container;  /* Id of container polygon that encloses this polygon (-1 if none) */
        int ancestor;   /* Id of ancestor polygon in the full resolution set that was the source of this polygon (-1 if none) */
};

//------------------------------------------------------------------------------
struct GSHHG_POINT {    /* Each lon, lat pair is stored in micro-degrees in 4-byte signed integer format */
    int32_t x;
    int32_t y;
};

//------------------------------------------------------------------------------
// Converts geocentric polar coordinates to geocentric cartesian coordinates
vec3f geocentric_to_cartesian(GSHHG_POINT const& p)
{
    constexpr float R = 6371008.8f;

    // Note that the 23-bit mantisaa of single-precision fp is insufficient
    // to represent the ~28 bits used for encoding lon/lat in micro-degrees.
    double lon = double(p.x) * (math::pi / 180000000.0);
    double lat = double(p.y) * (math::pi / 180000000.0);

    float cl = float(cos(lon)), sl = float(sin(lon));
    float cp = float(cos(lat)), sp = float(sin(lat));

    return {R * cl * cp, R * sl * cp, R * sp};
}

//------------------------------------------------------------------------------
// Converts WGS 84 geodetic coordinates to geocentric cartesian coordinates
vec3f wgs84_to_cartesian(GSHHG_POINT const& p)
{
    constexpr float A = 6378137.0f; //! Semi-major axis (equitorial radius)
    constexpr float B = 6356752.314245f; //! Semi-minor axis (polar radius)

    // Note that the 23-bit mantisaa of single-precision fp is insufficient
    // to represent the ~28 bits used for encoding lon/lat in micro-degrees.
    double lon = double(p.x) * (math::pi / 180000000.0);
    double lat = double(p.y) * (math::pi / 180000000.0);

    float cl = float(cos(lon)), sl = float(sin(lon));
    float cp = float(cos(lat)), sp = float(sin(lat));

    // Prime vertical radius of curvature
    float N = (A * A) / sqrt((A * A) * cp * cp + (B * B) * sp * sp);
    return {N * cl * cp, N * sl * cp, (B * B / (A * A)) * N * sp};
}

//------------------------------------------------------------------------------
// Converts WGS 84 geodetic coordinates to geocentric cartesian coordinates and then
// projects them onto a sphere with surface area equal to the reference ellipsoid.
vec3f wgs84_projection_to_cartesian(GSHHG_POINT const& p)
{
    constexpr float A = 6378137.0f; //! Semi-major axis (equitorial radius)
    constexpr float B = 6356752.314245f; //! Semi-minor axis (polar radius)
    constexpr float R = 6371008.8f; //! IUGG arithmetic mean radius of the Earth.

    // Note that the 23-bit mantisaa of single-precision fp is insufficient
    // to represent the ~28 bits used for encoding lon/lat in micro-degrees.
    double lon = double(p.x) * (math::pi / 180000000.0);
    double lat = double(p.y) * (math::pi / 180000000.0);

    float cl = float(cos(lon)), sl = float(sin(lon));
    float cp = float(cos(lat)), sp = float(sin(lat));

    vec3 r = vec3(cl * cp, sl * cp, (B * B / (A * A)) * sp).normalize();
    return {float(R * r.x), float(R * r.y), float(R * r.z)};
}

//------------------------------------------------------------------------------
gshhg::gshhg()
{
}

//------------------------------------------------------------------------------
bool gshhg::load(resolution res)
{
    constexpr char resolution_names[] = "fhilc";

    clear();

    if (res < resolution::full || res > resolution::crude) {
        return false;
    }

    file::stream b = file::open(va("assets/ref/gshhg-bin-2.3.7/gshhs_%c.b", resolution_names[static_cast<int>(res)]), file::mode::read);
    std::vector<int32_t> f;
    f.resize(b.size() / sizeof(int32_t));
    b.read(reinterpret_cast<file::byte*>(f.data()), f.size() * sizeof(int32_t));

    {
        // swap big-endian in-place
        unsigned char* ptr = reinterpret_cast<unsigned char*>(f.data());
        unsigned char* end = reinterpret_cast<unsigned char*>(f.data() + f.size());
        while (ptr < end) {
            unsigned char swap = ptr[0];
            ptr[0] = ptr[3];
            ptr[3] = swap;
            swap = ptr[1];
            ptr[1] = ptr[2];
            ptr[2] = swap;
            ptr += 4;
        }
    }

    {
        GSHHG const* ptr = reinterpret_cast<GSHHG const*>(f.data());
        GSHHG const* end = reinterpret_cast<GSHHG const*>(f.data() + f.size());
        while(ptr < end) {
            GSHHG_POINT const* pts = reinterpret_cast<GSHHG_POINT const*>(ptr + 1);
            _polygons.push_back(poly{narrow_cast<int>(_vertices.size()), ptr->n, ptr->flag});
            for (int ii = 0; ii < ptr->n; ++ii) {
                vec3f p = wgs84_projection_to_cartesian(pts[ii]);
                _vertices.push_back({p.x, p.y, p.z});
            }
            ptr = reinterpret_cast<GSHHG const*>(pts + ptr->n);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
void gshhg::clear()
{
    _vertices.clear();
    _polygons.clear();
}
