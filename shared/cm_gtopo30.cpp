// shared/cm_gtopo30.cpp
//

#include "cm_gtopo30.h"
#include "cm_string.h"
#include "cm_shared.h"
#include "cm_filesystem.h"

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Helper function that should probably exist in parser somewhere
std::size_t scan_integer(string::view text, string::view key)
{
    char const* s = strstr(text.c_str(), key.c_str());
    assert(s);
    s += key.length();
    while (*s <= 32) {
        ++s;
    }
    return atoi(s);
}

//------------------------------------------------------------------------------
// Helper function that should probably exist in parser somewhere
double scan_scalar(string::view text, string::view key)
{
    char const* s = strstr(text.c_str(), key.c_str());
    assert(s);
    s += key.length();
    while (*s <= 32) {
        ++s;
    }
    return atof(s);
}

//------------------------------------------------------------------------------
gtopo30::gtopo30()
    : _tiles{}
{
    std::size_t idx = 0;
    // Load 
    for (int lat = 90; lat > -60; lat -= 50) {
        for (int lng = -180; lng < 180; lng += 40) {
            load_tile(lng, lat, _tiles[idx++]);
        }
    }

    for (int lng = -180; lng < 180; lng += 60) {
        load_tile(lng, -60, _tiles[idx++]);
    }
}

//------------------------------------------------------------------------------
float gtopo30::height(vec3 v) const
{
    // TODO: use WGS84 to convert to long/lat
    float longitude = math::rad2deg(atan2f(v.y, v.x));
    float latitude = math::rad2deg(atan2f(v.z, sqrt(v.x * v.x + v.y * v.y)));

    vec3 grid[4] = {};

    for (std::size_t ii = 0; ii < countof(_tiles); ++ii) {
        std::size_t x = std::ptrdiff_t((longitude - _tiles[ii].ulxmap) / _tiles[ii].xdim);
        std::size_t y = std::ptrdiff_t((_tiles[ii].ulymap - latitude) / _tiles[ii].ydim);

        if (x < _tiles[ii].ncols && y < _tiles[ii].nrows) {
            grid[0].x = float(_tiles[ii].ulxmap + _tiles[ii].xdim * x);
            grid[0].y = float(_tiles[ii].ulymap - _tiles[ii].ydim * y);
            grid[0].z = _tiles[ii].data[y * _tiles[ii].ncols + x];
        }

        if ((x + 1) < _tiles[ii].ncols && y < _tiles[ii].nrows) {
            grid[1].x = float(_tiles[ii].ulxmap + _tiles[ii].xdim * (x + 1));
            grid[1].y = float(_tiles[ii].ulymap - _tiles[ii].ydim * y);
            grid[1].z = _tiles[ii].data[y * _tiles[ii].ncols + x + 1];
        }

        if (x < _tiles[ii].ncols && (y + 1) < _tiles[ii].nrows) {
            grid[2].x = float(_tiles[ii].ulxmap + _tiles[ii].xdim * x);
            grid[2].y = float(_tiles[ii].ulymap - _tiles[ii].ydim * (y + 1));
            grid[2].z = _tiles[ii].data[(y + 1) * _tiles[ii].ncols + x];
        }

        if ((x + 1) < _tiles[ii].ncols && (y + 1) < _tiles[ii].nrows) {
            grid[3].x = float(_tiles[ii].ulxmap + _tiles[ii].xdim * (x + 1));
            grid[3].y = float(_tiles[ii].ulymap - _tiles[ii].ydim * (y + 1));
            grid[3].z = _tiles[ii].data[(y + 1) * _tiles[ii].ncols + x + 1];
        }
    }

    // Bilinear interpolation
    float s = (longitude - grid[0].x) / (grid[1].x - grid[0].x);
    float t = (latitude - grid[0].y) / (grid[2].y - grid[0].y);

    return (grid[0].z * (1.f - s) + grid[1].z * s) * (1.f - t)
         + (grid[2].z * (1.f - s) + grid[3].z * s) * t;
}

//------------------------------------------------------------------------------
void gtopo30::load_tile(int lng, int lat, tile& tile) const
{
    string::view filename = va("%c%03d%c%02d",
        lng <= 0 ? 'W' : 'E',
        lng <= 0 ? -lng : lng,
        lat < 0 ? 'S' : 'N',
        lat < 0 ? -lat : lat);

    file::buffer hdr = file::read(va("assets/ref/gtopo30/%s.HDR", filename.c_str()));
    string::view hdr_text(reinterpret_cast<char const*>(hdr.data()),
                          reinterpret_cast<char const*>(hdr.data() + hdr.size()));

    tile.nrows = scan_integer(hdr_text, "NROWS");
    tile.ncols = scan_integer(hdr_text, "NCOLS");
    tile.ulxmap = scan_scalar(hdr_text, "ULXMAP");
    tile.ulymap = scan_scalar(hdr_text, "ULYMAP");
    tile.xdim = scan_scalar(hdr_text, "XDIM");
    tile.ydim = scan_scalar(hdr_text, "YDIM");

    file::stream dem = file::open(va("assets/ref/gtopo30/%s.DEM", filename.c_str()), file::mode::read);
    tile.data.resize(tile.nrows * tile.ncols);
    dem.read(reinterpret_cast<file::byte*>(tile.data.data()), tile.nrows * tile.ncols * sizeof(int16_t));
    // swap big-endian in-place
    unsigned char* ptr = reinterpret_cast<unsigned char*>(tile.data.data());
    unsigned char* end = reinterpret_cast<unsigned char*>(tile.data.data() + tile.data.size());
    while (ptr < end) {
        unsigned char swap = ptr[0];
        ptr[0] = ptr[1];
        ptr[1] = swap;
        ptr += 2;
    }
}
