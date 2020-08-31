// r_font.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_filesystem.h"
#include "cm_unicode.h"
#include "win_include.h"
#include <GL/gl.h>

////////////////////////////////////////////////////////////////////////////////
namespace {

struct unicode_block {
    char32_t from;      //!< First codepoint in this code block
    char32_t to;        //!< Last codepoint in this code block
    char const* name;   //!< Name of this code block
};

struct unicode_block_layout : unicode_block {
    int offset;         //!< Number of characters in all preceding code blocks
    int size;           //!< Number of characters in this code block
};

#pragma region Shenanigans for compile-time instantiation

//------------------------------------------------------------------------------
template<int size>
constexpr int unicode_block_size(int index, unicode_block const (&blocks)[size])
{
    return blocks[index].to - blocks[index].from + 1;
}

//------------------------------------------------------------------------------
template<int size>
constexpr int unicode_block_offset(int index, unicode_block const (&blocks)[size])
{
    return index > 0
        ? unicode_block_offset(index - 1, blocks) + unicode_block_size(index - 1, blocks)
        : 0;
}

//------------------------------------------------------------------------------
template<int... Is> struct indices{};
template<int N, int... Is> struct build_indices : build_indices<N - 1, N - 1, Is...>{};
template<int... Is> struct build_indices<0, Is...> : indices<Is...>{};

//------------------------------------------------------------------------------
template<int size>
constexpr unicode_block_layout build_unicode_block_layout(unicode_block const (&blocks)[size], int index)
{
    return {
        blocks[index].from,
        blocks[index].to,
        blocks[index].name,
        unicode_block_offset(index, blocks),
        unicode_block_size(index, blocks),
    };
}

//------------------------------------------------------------------------------
template<int size, int... Is>
constexpr std::array<unicode_block_layout, size> build_unicode_block_layout(unicode_block const (&blocks)[size], indices<Is...>)
{
    return {{ build_unicode_block_layout<size>(blocks, Is)... }};
}

//------------------------------------------------------------------------------
template<int size>
constexpr std::array<unicode_block_layout, size> build_unicode_block_layout(unicode_block const (&blocks)[size])
{
    return build_unicode_block_layout<size>(blocks, build_indices<size>{});
}

#pragma endregion

//------------------------------------------------------------------------------
template<int sz> class unicode_block_data
{
public:
    static constexpr char32_t invalid_codepoint = 0xffff; //!< Not a valid character
    static constexpr char32_t unknown_codepoint = 0xfffd; //!< Replacement character
    static constexpr int invalid_index = -1; //!< Not a valid character index

public:
    constexpr unicode_block_data(unicode_block const (&blocks)[sz])
        : _blocks(build_unicode_block_layout(blocks))
    {}

    //! Returns an iterator to the beginning of the Unicode block layouts
    constexpr auto begin() const { return _blocks.begin(); }

    //! Returns an iterator to the end of the Unicode block layouts
    constexpr auto end() const { return _blocks.end(); }

    //! Returns the number of loaded Unicode blocks
    constexpr int size() const { return sz; }

    //! Returns the Unicode block layout at the given index
    constexpr unicode_block_layout operator[](int index) const { return _blocks[index]; }

    //! Returns the maximum valid character index
    constexpr int max_index() const { return _blocks.back().offset + _blocks.back().size - 1; }

    //! Returns character index for the given codepoint
    constexpr int codepoint_to_index(char32_t codepoint) const {
        for (int ii = 0; ii < sz && _blocks[ii].from <= codepoint; ++ii) {
            if (_blocks[ii].from <= codepoint && codepoint <= _blocks[ii].to) {
                return codepoint - _blocks[ii].from + _blocks[ii].offset;
            }
        }
        return codepoint_to_index(unknown_codepoint);
    }

    //! Returns codepoint for the given character index
    constexpr char32_t index_to_codepoint(int index) const {
        for (int ii = 0; ii < sz && _blocks[ii].offset <= index; ++ii) {
            if (_blocks[ii].offset <= index && index < _blocks[ii].offset + _blocks[ii].size) {
                return _blocks[ii].from + index - _blocks[ii].offset;
            }
        }
        return unknown_codepoint;
    }

    //! Returns true if `s` points to an ASCII character
    static constexpr bool is_ascii(char const* s) {
        return unicode::is_ascii(s);
    }

    //! Returns true if `s` points to a valid UTF-8 character
    static constexpr bool is_utf8(char const* s) {
        return unicode::is_utf8(s);
    }

    //! Returns the decoded codepoint for the UTF-8 character at `s`
    //! and advances the pointer to the next character sequence.
    static constexpr char32_t decode(char const*& s) {
        return unicode::decode(s);
    }

    //! Returns the character index for the UTF-8 character at `s`
    //! and advances the pointer to the next character sequence.
    constexpr int decode_index(char const*& s) const {
        return codepoint_to_index(decode(s));
    }

protected:
    std::array<unicode_block_layout, sz> _blocks;
};

constexpr unicode_block_data unicode_data(
    {
        { 0x0000, 0x007f, "Basic Latin" },
        { 0x0080, 0x00ff, "Latin-1 Supplement" },
        { 0x0100, 0x017f, "Latin Extended-A" },
        { 0x0180, 0x024f, "Latin Extended-B" },
        // { 0x0250, 0x02af, "IPA Extensions" },
        // { 0x02b0, 0x02ff, "Spacing Modifier Letters" },
        // { 0x0300, 0x036f, "Combining Diacritic Marks" },
        { 0x0370, 0x03ff, "Greek and Coptic" },
        { 0x0400, 0x04ff, "Cyrillic" },
        { 0x0500, 0x052f, "Cyrillic Supplement" },
        { 0x0530, 0x058f, "Armenian" },
        { 0x0590, 0x05ff, "Hebrew" },
        { 0x0600, 0x06ff, "Arabic" },
        // ...
        { 0x2000, 0x206f, "General Punctuation" },
        // ...
        { 0x3040, 0x309f, "Hiragana" },
        // ...
        { 0xfe70, 0xfeff, "Arabic Presentation Forms-B" },
        // { 0xff00, 0xffef, "Halfwidth and Fullwidth Forms" },
        { 0xfff0, 0xffff, "Specials" },
    }
);

#define STATIC_ASSERT_ROUNDTRIP(n)                                              \
static_assert(unicode_data.index_to_codepoint(unicode_data.codepoint_to_index(n)) == n, "round trip failed for Unicode codepoint " #n)
STATIC_ASSERT_ROUNDTRIP(0x03bc); // mu                      [Greek and Coptic]
STATIC_ASSERT_ROUNDTRIP(0x0643); // kaf                     [Arabic]
STATIC_ASSERT_ROUNDTRIP(0xfed9); // kaf isolated form       [Arabic Presentation Forms-B]
STATIC_ASSERT_ROUNDTRIP(0xfffd); // replacement character   [Specials]
#undef STATIC_ASSERT_ROUNDTRIP

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
namespace render {

HFONT font::_system_font = NULL;
HFONT font::_active_font = NULL;

//------------------------------------------------------------------------------
struct glyph
{
    std::vector<vec2> points;
    struct range { std::size_t first, last; };
    std::vector<range> edges; // indexes into points
    std::vector<range> contours; // indexes into edges
    // channel mask for each edge
    // all edges belong to at least two channels
    // all adjacent edges share one channel in common
    std::vector<uint8_t> edge_channels;
};

//------------------------------------------------------------------------------
float determinant(vec2 a, vec2 b)
{
    float cd = a.y * b.x;
    float err = std::fma(-a.y, b.x, cd);
    return std::fma(a.x, b.y, -cd) + err;
}

//------------------------------------------------------------------------------
float distance_sqr(vec2 a, vec2 b)
{
    // naive: length_sqr(a - b)
    float xs = a.x - b.x;
    float xe = (std::abs(a.x) > std::abs(b.x))
                ? static_cast<float>(-b.x - xs) + a.x
                : static_cast<float>(a.x - xs) - b.x;
    float ys = a.y - b.y;
    float ye = (std::abs(a.y) > std::abs(b.y))
                ? static_cast<float>(-b.y - ys) + a.y
                : static_cast<float>(a.y - ys) - b.y;

    assert(xe != 0.f || a.x == (xs + b.x));
    assert(xe != 0.f || b.x == (a.x - xs));
    assert(ye != 0.f || a.y == (ys + b.y));
    assert(ye != 0.f || b.y == (a.y - ys));

    // x^2 + y^2 = (xs + xe)^2 + (ys + ye)^2
    //           = xs^2 + 2xsxe + xe^2 + ys^2 + 2ysye + ye^2
    const float terms[6] = {
        xs * xs,
        xs * xe * 2.f,
        xe * xe,
        ys * ys,
        ys * ye * 2.f,
        ye * ye
    };

    float s = 0.f, e = 0.f;
    for (int ii = 0; ii < countof(terms); ++ii) {
        float t = s;
        float y = terms[ii] + e;
        s = t + y;
        e = float(t - s) + y;
    }

    {
        double x = double(a.x) - double(b.x);
        double y = double(a.y) - double(b.y);
        double dd = x * x + y * y;
        double ds = (a - b).length_sqr();
        assert(std::abs(dd - double(s)) <= std::abs(dd - ds));
        if (std::abs(dd - double(s)) < std::abs(dd - ds)) {
            __debugbreak();
        }
    }

    return s;
}

struct edge_distance
{
    float signed_distance_sqr;
    float orthogonal_distance;

    edge_distance operator-() const {
        return {-signed_distance_sqr, orthogonal_distance};
    }
    bool operator<(edge_distance const& other) const {
        return std::abs(signed_distance_sqr) < std::abs(other.signed_distance_sqr)
            || std::abs(signed_distance_sqr) == std::abs(other.signed_distance_sqr)
                && orthogonal_distance > other.orthogonal_distance;
    }
    bool operator==(edge_distance const& other) const {
        return signed_distance_sqr == other.signed_distance_sqr
            && orthogonal_distance == other.orthogonal_distance;
    }
};

//------------------------------------------------------------------------------
edge_distance signed_segment_distance_squared(vec2 a, vec2 b, vec2 c)
{
    vec2 v = b - a;
    vec2 r = c - a;
    float num = dot(v, r);
    float den = dot(v, v);
    float det = determinant(v, r);
    float ortho = det * det / den;
    if (num >= den) {
        return {std::copysign((c - b).length_sqr(), det), ortho};
    } else if (num < 0.f) {
        return {std::copysign((c - a).length_sqr(), det), ortho};
    } else {
        return {std::copysign(det * det / den, det), ortho};
    }
}

//------------------------------------------------------------------------------
edge_distance signed_ray_distance_squared(vec2 a, vec2 b, vec2 c)
{
    vec2 v = b - a;
    vec2 r = c - a;
    float num = dot(v, r);
    float den = dot(v, v);
    float det = determinant(v, r);
    float ortho = det * det / den;
    if (num < den) {
        return {std::copysign((c - b).length_sqr(), det), ortho};
    } else {
        return {std::copysign(det * det / den, det), ortho};
    }
}

//------------------------------------------------------------------------------
edge_distance signed_edge_distance_squared(glyph const& glyph, vec2 p, std::size_t edge_index)
{
    edge_distance sdsqr_min = {FLT_MAX, -FLT_MAX};
    std::size_t first = glyph.edges[edge_index].first;
    std::size_t last = glyph.edges[edge_index].last;
    for (std::size_t ii = first; ii < last; ++ii) {
        edge_distance sdsqr = signed_segment_distance_squared(glyph.points[ii], glyph.points[ii + 1], p);
        if (sdsqr < sdsqr_min) {
            sdsqr_min = sdsqr;
        }
    }
    return sdsqr_min;
}

//------------------------------------------------------------------------------
// signed edge distance except the edge segments at endpoints are extruded to infinity
float signed_edge_pseudo_distance(glyph const& glyph, vec2 p, std::size_t edge_index)
{
    edge_distance sdsqr_min = signed_edge_distance_squared(glyph, p, edge_index);
#if 1
    std::size_t first = glyph.edges[edge_index].first;
    std::size_t last = glyph.edges[edge_index].last;

    // only compare extruded edge segments if edge is not a loop
    if (glyph.points[first] != glyph.points[last]) {
        // compare signed distance to ray extending from first segment of edge if first segment is closest
        edge_distance sdsqr_first0 = signed_segment_distance_squared(glyph.points[first], glyph.points[first + 1], p);
        edge_distance sdsqr_first = -signed_ray_distance_squared(glyph.points[first + 1], glyph.points[first], p);
        if (sdsqr_first0 == sdsqr_min && sdsqr_first < sdsqr_min) {
            sdsqr_min = sdsqr_first;
        }
        // compare signed distance from ray extending from last segment of edge if last segment is closest
        edge_distance sdsqr_last0 = signed_segment_distance_squared(glyph.points[last - 1], glyph.points[last], p);
        edge_distance sdsqr_last = signed_ray_distance_squared(glyph.points[last - 1], glyph.points[last], p);
        if (sdsqr_last0 == sdsqr_min && sdsqr_last < sdsqr_min) {
            sdsqr_min = sdsqr_last;
        }
    }
#endif
    return std::copysign(std::sqrt(std::abs(sdsqr_min.signed_distance_sqr)), sdsqr_min.signed_distance_sqr);
}

//------------------------------------------------------------------------------
float median(vec3 x)
{
    if (x.x > x.z) std::swap(x.x, x.z);
    if (x.x > x.y) std::swap(x.x, x.y);
    if (x.y > x.z) std::swap(x.y, x.z);
    assert(x.x <= x.y && x.y <= x.z);
    return x.y;
}

//------------------------------------------------------------------------------
bool intersect_segments(vec2 a, vec2 b, vec2 c, vec2 d)
{
    vec2 ab = b - a;
    vec2 cd = d - c;
    float den = determinant(ab, cd);
    if (den == 0.f) {
        return false;
    }
    vec2 ac = c - a;
    float sgn = den < 0.f ? -1.f : 1.f;
    den = std::abs(den);
    float u = determinant(ac, cd) * sgn;
    float v = determinant(ac, ab) * sgn;
    return (0.f <= u && u <= den && 0.f <= v && v <= den);
}

//------------------------------------------------------------------------------
bool point_inside_glyph(glyph const& glyph, vec2 p)
{
    std::size_t n = 0;
    // If the line from the point to an arbitrary point outside the glyph crosses
    // the edges of the glyph an odd number of times then the point is inside.
    for (std::size_t ii = 0, sz = glyph.edges.size(); ii < sz; ++ii) {
        for (std::size_t jj = glyph.edges[ii].first; jj < glyph.edges[ii].last; ++jj) {
            if (intersect_segments(p, vec2(-32.f, -16.f), glyph.points[jj], glyph.points[jj + 1])) {
                ++n;
            }
        }
    }
    return n % 2 == 1;
}

//------------------------------------------------------------------------------
vec3 signed_edge_distance_channels(glyph const& glyph, vec2 p)
{
    edge_distance dsqr_min[3] = {{FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}};
    std::size_t emin[3] = {};

    // find closest edge for each channel
    for (std::size_t ii = 0, sz = glyph.edges.size(); ii < sz; ++ii) {
        edge_distance dsqr = signed_edge_distance_squared(glyph, p, ii);
        for (std::size_t ch = 0; ch < 3; ++ch) {
            if ((glyph.edge_channels[ii] & (1 << ch)) && dsqr < dsqr_min[ch]) {
                dsqr_min[ch] = dsqr;
                emin[ch] = ii;
            }
        }
    }

    vec3 dist = vec3(
        signed_edge_pseudo_distance(glyph, p, emin[0]),
        signed_edge_pseudo_distance(glyph, p, emin[1]),
        signed_edge_pseudo_distance(glyph, p, emin[2])
    );
#if 0
    if (median(dist) < 0.f && !point_inside_glyph(glyph, p)) {
        int mind = 0;
        int maxd = 0;
        for (int ii = 1; ii < 3; ++ii) {
            if (dist[ii] < dist[mind]) {
                mind = ii;
            }
            if (dist[ii] > dist[maxd]) {
                maxd = ii;
            }
        }
        dist[mind] = dist[maxd];
    }
#endif
    return dist;
}

//------------------------------------------------------------------------------
float signed_edge_distance(glyph const& glyph, vec2 p)
{
#if 1
    edge_distance dsqr_min = {FLT_MAX, -FLT_MAX};
    std::size_t emin = {};

    // find closest edge
    for (std::size_t ii = 0, sz = glyph.edges.size(); ii < sz; ++ii) {
        edge_distance dsqr = signed_edge_distance_squared(glyph, p, ii);
        if (dsqr < dsqr_min) {
            dsqr_min = dsqr;
            emin = ii;
        }
    }

    return signed_edge_pseudo_distance(glyph, p, emin);
    //return std::copysign(std::sqrt(std::abs(dsqr_min.signed_distance_sqr)), dsqr_min.signed_distance_sqr);
#else
    edge_distance dsqr_min = signed_segment_distance_squared(glyph.points[0], glyph.points[1], p);
    return std::copysign(std::sqrt(std::abs(dsqr_min.signed_distance_sqr)), dsqr_min.signed_distance_sqr);
#endif
}

//------------------------------------------------------------------------------
std::size_t signed_edge_index(glyph const& glyph, vec2 p)
{
    edge_distance dsqr_min = {FLT_MAX, -FLT_MAX};
    std::size_t emin = {};

    // find closest edge
    for (std::size_t ii = 0, sz = glyph.edges.size(); ii < sz; ++ii) {
        edge_distance dsqr = signed_edge_distance_squared(glyph, p, ii);
        if (dsqr < dsqr_min) {
            dsqr_min = dsqr;
            emin = ii;
        }
    }

    return emin;
}

//------------------------------------------------------------------------------
void generate_bitmap(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint8_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
            vec3 d = signed_edge_distance_channels(glyph, point);
            // discretize d and write to bitmap ...
            data[yy * row_stride + xx * 3 + 0] = clamp<uint8_t>(d[0] * (255.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 1] = clamp<uint8_t>(d[1] * (255.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 2] = clamp<uint8_t>(d[2] * (255.f / 1.f), 0, 255);
        }
    }
}

//------------------------------------------------------------------------------
void generate_channel_bitmap(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint8_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
            vec3 d = signed_edge_distance_channels(glyph, point);
            // discretize d and write to bitmap ...
            data[yy * row_stride + xx * 3 + 0] = 255 - clamp<uint8_t>(std::abs(d[0]) * (1024.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 1] = 255 - clamp<uint8_t>(std::abs(d[1]) * (1024.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 2] = 255 - clamp<uint8_t>(std::abs(d[2]) * (1024.f / 1.f), 0, 255);
        }
    }
}

//------------------------------------------------------------------------------
void generate_median_bitmap(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint8_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
            vec3 d = signed_edge_distance_channels(glyph, point);
            // discretize d and write to bitmap ...
            float m = median(d);
            data[yy * row_stride + xx * 3 + 0] = clamp<uint8_t>(m * (4096.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 1] = clamp<uint8_t>(m * (4096.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 2] = clamp<uint8_t>(m * (4096.f / 1.f), 0, 255);
        }
    }
}

//------------------------------------------------------------------------------
void generate_mask_bitmap(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint8_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
            std::size_t index = signed_edge_index(glyph, point);

            uint8_t r = ((index + 1) *  61) % 255;
            uint8_t g = ((index + 1) * 103) % 255;
            uint8_t b = ((index + 1) * 173) % 255;

            if (point_inside_glyph(glyph, point)) {
                r /= 2;
                g /= 2;
                b /= 2;
            }

            data[yy * row_stride + xx * 3 + 0] = r;
            data[yy * row_stride + xx * 3 + 1] = g;
            data[yy * row_stride + xx * 3 + 2] = b;
        }
    }
}

//------------------------------------------------------------------------------
void generate_rings_bitmap(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint8_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
#if 0
            vec3 d = signed_edge_distance_channels(glyph, point);
            float m = std::fmod(median(d), .5f);
#else
            float m = std::fmod(signed_edge_distance(glyph, point), .5f);
#endif
            data[yy * row_stride + xx * 3 + 0] = clamp<uint8_t>(m * (512.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 1] = clamp<uint8_t>(m * (512.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 2] = clamp<uint8_t>(m * (512.f / 1.f), 0, 255);
        }
    }
}

//------------------------------------------------------------------------------
void write_bitmap(string::view filename, std::size_t image_width, std::size_t image_height, std::vector<uint8_t> const& image_data)
{
    BITMAPINFOHEADER bmi{};
    bmi.biSize = sizeof(bmi);
    bmi.biWidth = narrow_cast<LONG>(image_width);
    bmi.biHeight = narrow_cast<LONG>(image_height);
    bmi.biPlanes = 1;
    bmi.biCompression = BI_RGB;
    bmi.biBitCount = 24;
    bmi.biSizeImage = 0;
    bmi.biXPelsPerMeter = 0;
    bmi.biYPelsPerMeter = 0;
    bmi.biClrUsed = 0;
    bmi.biClrImportant = 0;

    BITMAPFILEHEADER hdr{};

    hdr.bfType = 0x4d42;
    hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_width * image_height * 3);
    hdr.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

    auto f = file::open(filename, file::mode::write); 
    f.write((file::byte const*)&hdr, sizeof(hdr));
    f.write((file::byte const*)&bmi, sizeof(bmi));
    f.write(image_data.data(), image_data.size());
    f.close();
}

//------------------------------------------------------------------------------
void subdivide_qspline(std::vector<vec2>& points, vec2 p0, vec2 p1, vec2 p2, float chordalDeviationSquared)
{
    // cf. MakeLinesFromArc

    // de Casteljau's algorithm
    vec2 p01 = .5f * (p0 + p1);
    vec2 p12 = .5f * (p1 + p2);
    vec2 mid = .5f * (p01 + p12); // on the spline

    vec2 dx = mid - p1;
    if (dx.length_sqr() > chordalDeviationSquared) {
        subdivide_qspline(points, p0, p01, mid, chordalDeviationSquared);
        subdivide_qspline(points, mid, p12, p2, chordalDeviationSquared);
    } else {
        points.push_back(p1);
    }
}

//------------------------------------------------------------------------------
glyph rasterize_glyph(HDC hdc, UINT ch, float chordalDeviationSquared = 0.f)
{
    glyph g;

    // /onecoreuap/windows/opengl/client/fontoutl.c

    // cf. ScaleFont
    OUTLINETEXTMETRICW otm{};

    GetOutlineTextMetricsW(hdc, sizeof(otm), &otm);
    if (chordalDeviationSquared == 0.f) {
        chordalDeviationSquared = 1.f / square(float(otm.otmEMSquare));
    }

    // cf. wglUseFontOutlinesAW
    GLYPHMETRICS gm{};
    MAT2 mat = {
        {0, 1}, {0, 0},
        {0, 0}, {0, 1},
    };

    DWORD glyphBufSize = GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, 0, NULL, &mat);
    std::vector<std::byte> glyphBuf(glyphBufSize);
    std::vector<vec2> points;

    GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, glyphBufSize, glyphBuf.data(), &mat);

    // cf. MakeLinesFromGlyph
    TTPOLYGONHEADER const* ph = reinterpret_cast<TTPOLYGONHEADER const*>(glyphBuf.data());
    TTPOLYGONHEADER const* hend = reinterpret_cast<TTPOLYGONHEADER const*>(glyphBuf.data() + glyphBufSize);
    while (ph < hend) {

        // cf. MakeLinesFromTTPolygon
        if (ph->dwType != TT_POLYGON_TYPE) {
            return {};
        }

        auto const GetFixedPoint = [](POINTFX const& pfx) {
            return vec2(float(pfx.x.value) + pfx.x.fract * (1.f / 65536.f),
                        float(pfx.y.value) + pfx.y.fract * (1.f / 65536.f));
        };

        std::size_t pstart = points.size();
        points.push_back(GetFixedPoint(ph->pfxStart));

        TTPOLYCURVE const* pc = reinterpret_cast<TTPOLYCURVE const*>(&ph[1]);
        TTPOLYCURVE const* cend = reinterpret_cast<TTPOLYCURVE const*>(reinterpret_cast<std::byte const*>(ph) + ph->cb);

        while (pc < cend) {
            // cf. MakeLinesFromTTPolycurve
            if (pc->wType == TT_PRIM_LINE) {
                // cf. MakeLinesFromTTLine
                for (WORD ii = 0; ii < pc->cpfx; ++ii) {
                    points.push_back(GetFixedPoint(pc->apfx[ii]));
                }
            } else if (pc->wType == TT_PRIM_QSPLINE) {
                // cf. MakeLinesFromTTQSpline
                vec2 p2, p1, p0 = points.back();
                for (WORD ii = 0; ii < pc->cpfx - 1; ++ii) {
                    p1 = GetFixedPoint(pc->apfx[ii]);
                    p2 = GetFixedPoint(pc->apfx[ii + 1]);
                    if (ii != pc->cpfx - 2) {
                        p2 = .5f * (p1 + p2);
                    }
                    subdivide_qspline(points, p0, p1, p2, chordalDeviationSquared);
                    p0 = p2;
                }

                points.push_back(p2);

            } else if (pc->wType == TT_PRIM_CSPLINE) {
                // Cubic B-splines are only returned from GetGlyphOutline with GGO_BEZIER
                return {};
            } else {
                return {};
            }

            pc = reinterpret_cast<TTPOLYCURVE const*>(&pc->apfx[pc->cpfx]);
        }

#if 1
        for (std::size_t ii = pstart; ii < points.size(); ++ii) {
            OutputDebugStringA(va("%g, %g\n\0", points[ii].x, points[ii].y).begin());
        }
        OutputDebugStringA("\n");
#endif

        if (points[pstart] != points.back()) {
            points.push_back(points[pstart]);
        }

        std::vector<std::size_t> corners;
        constexpr float cos_15 = 0.96592582628f;
        {
            vec2 v1 = (points[pstart + 0] - points[points.size() - 2]).normalize();
            vec2 v2 = (points[pstart + 1] - points[pstart + 0]).normalize();
            if (dot(v1, v2) < cos_15) {
                corners.push_back(pstart);
            }
        }
        for (std::size_t ii = pstart + 2; ii < points.size(); ++ii) {
            vec2 v1 = (points[ii - 1] - points[ii - 2]).normalize();
            vec2 v2 = (points[ii    ] - points[ii - 1]).normalize();
            if (dot(v1, v2) < cos_15) {
                corners.push_back(ii - 1);
            }
        }

        // in the case where there is only one corner the edge needs to be subdivided
        if (corners.size() == 1) {
            // split the edge on the point furthest from the corner (euclidean distance, not arc distance)
            float dsqr = 0.f;
            std::size_t pmax = corners[0];
            for (std::size_t ii = pstart; ii < points.size(); ++ii) {
                float d = (points[corners[0]] - points[ii]).length_sqr();
                if (d > dsqr) {
                    dsqr = d;
                    pmax = ii;
                }
            }
            corners.push_back(pmax);
        }
#if 1
        for (std::size_t ii = 0; ii < corners.size(); ++ii) {
            OutputDebugStringA(va("    %g, %g\n\0", points[corners[ii]].x, points[corners[ii]].y).begin());
        }
        OutputDebugStringA("\n");
#endif

        constexpr uint8_t masks[3] = { 0b011, 0b101, 0b110 };

        // add contour to glyph
        g.contours.push_back({g.edges.size(),
                              g.edges.size() + max<std::size_t>(1, corners.size()) - 1});
        // rotate points so that they start on a corner (if one exists)
        if (corners.size()) {
            std::size_t pivot = corners[0];
            std::size_t pcount = points.size() - pstart - 1; // ignore duplicate start point
            for (std::size_t ii = 0; ii < pcount; ++ii) {
                // rotate left by (pivot - pstart)
                g.points.push_back(points[pstart + (pivot - pstart + ii) % pcount]);
            }
            assert(g.points[pstart] == points[pivot]);
            // add rotated start point as duplicate
            g.points.push_back(points[pivot]);
            std::size_t shift_right = pivot - pstart;
            std::size_t shift_left = pcount - shift_right;
            for (std::size_t ii = 0; ii < corners.size() - 1; ++ii) {
                // index of rotated points
                g.edge_channels.push_back(masks[g.edges.size() % 3]);
                g.edges.push_back({pstart + (shift_left + corners[ii] - pstart) % pcount,
                                   pstart + (shift_left + corners[ii + 1] - pstart) % pcount});
                assert(g.points[g.edges.back().first] == points[corners[ii]]);
                assert(g.points[g.edges.back().last] == points[corners[ii + 1]]);
            }
            // add final edge with duplicated start point
            g.edge_channels.push_back(masks[g.edges.size() % 3]);
            if (g.edge_channels.back() == g.edge_channels[g.contours.back().first]) {
                g.edge_channels.back() = masks[(g.edges.size() + 1) % 3];
            }
            g.edges.push_back({pstart + (shift_left + corners[corners.size() - 1] - pstart) % pcount,
                               g.points.size() - 1});
            assert(g.points.size() - 1 == pstart + pcount);
            assert(g.points[g.edges.back().first] == points[corners[corners.size() - 1]]);
            assert(g.points[g.edges.back().last] == points[pivot]);
        } else {
            std::size_t pcount = points.size() - pstart;
            for (std::size_t ii = 0; ii < pcount; ++ii) {
                g.points.push_back(points[pstart + ii]);
            }
            // add a single edge containing all points
            g.edge_channels.push_back(masks[g.edges.size() % 3]);
            g.edges.push_back({pstart, pstart + pcount - 1});
        }

        ph = reinterpret_cast<TTPOLYGONHEADER const*>(cend);
    }

#if 1
    {
        for (std::size_t jj = 0, sz = g.edges.size(); jj < sz; ++jj) {
                OutputDebugStringA("edge\n");
            for (std::size_t ii = g.edges[jj].first; ii <= g.edges[jj].last; ++ii) {
                OutputDebugStringA(va("    %g, %g\n\0", g.points[ii].x, g.points[ii].y).begin());
            }
            OutputDebugStringA("\n");
        }
    }
#endif

    return g;
}

//------------------------------------------------------------------------------
void foo(HDC hdc, UINT ch, float chordalDeviationSquared = 0.f)
{
    glyph g = rasterize_glyph(hdc, ch, chordalDeviationSquared);

    GLYPHMETRICS gm{};
    MAT2 mat = {
        {0, 1}, {0, 0},
        {0, 0}, {0, 1},
    };

    GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, 0, NULL, &mat);

    constexpr std::size_t image_width = 256;
    constexpr std::size_t image_height = 256;

    float sx = float(max(gm.gmBlackBoxX + 2, gm.gmBlackBoxY + 2)) / float(image_width);
    float sy = float(max(gm.gmBlackBoxX + 2, gm.gmBlackBoxY + 2)) / float(image_height);

    bounds b = bounds::from_points(g.points.data(), g.points.size());

    mat3 image_to_glyph = mat3(sx, 0.f, 0.f,
                               0.f, sy, 0.f,
                               //-float(gm.gmptGlyphOrigin.x) / sx, -float(gm.gmptGlyphOrigin.y - gm.gmBlackBoxY) / sy, 1.f);
                               b[0][0] - 1.f, b[0][1] - 1.f, 1.f);
    std::vector<std::uint8_t> image_data(image_width * image_height * 3);
    char s[8] = {};
    if (ch < 128) {
        s[0] = (char)ch;
        s[1] = 0;
    } else {
        sprintf_s(s, "U%04X", ch);
    }

    generate_bitmap(g, image_to_glyph, image_width, image_height, image_width * 3, image_data.data());
    write_bitmap(va("sdf/%s.bmp", s), image_width, image_height, image_data);

    generate_channel_bitmap(g, image_to_glyph, image_width, image_height, image_width * 3, image_data.data());
    write_bitmap(va("sdf/%s_channels.bmp", s), image_width, image_height, image_data);

    generate_mask_bitmap(g, image_to_glyph, image_width, image_height, image_width * 3, image_data.data());
    write_bitmap(va("sdf/%s_mask.bmp", s), image_width, image_height, image_data);

    generate_median_bitmap(g, image_to_glyph, image_width, image_height, image_width * 3, image_data.data());
    write_bitmap(va("sdf/%s_median.bmp", s), image_width, image_height, image_data);

    generate_rings_bitmap(g, image_to_glyph, image_width, image_height, image_width * 3, image_data.data());
    write_bitmap(va("sdf/%s_rings.bmp", s), image_width, image_height, image_data);
}

//------------------------------------------------------------------------------
render::font const* system::load_font(string::view name, int size)
{
    for (auto const& f : _fonts) {
        if (f->compare(name, size)) {
            return f.get();
        }
    }
    _fonts.push_back(std::make_unique<render::font>(name, size));
    return _fonts.back().get();
}

//------------------------------------------------------------------------------
font::font(string::view name, int size)
    : _name(name)
    , _size(size)
    , _handle(NULL)
    , _list_base(0)
    , _char_width{0}
{
    GLYPHMETRICS gm;
    MAT2 m;

    // allocate lists
    _list_base = glGenLists(unicode_data.max_index() + 1);
    _char_width.resize(unicode_data.max_index() + 1);

    // create font
    _handle = CreateFontA(
        _size,                      // cHeight
        0,                          // cWidth
        0,                          // cEscapement
        0,                          // cOrientation
        FW_NORMAL,                  // cWeight
        FALSE,                      // bItalic
        FALSE,                      // bUnderline
        FALSE,                      // bStrikeOut
        ANSI_CHARSET,               // iCharSet
        OUT_TT_PRECIS,              // iOutPrecision
        CLIP_DEFAULT_PRECIS,        // iClipPrecision
        ANTIALIASED_QUALITY,        // iQuality
        FF_DONTCARE|DEFAULT_PITCH,  // iPitchAndFamily
        _name.c_str()               // pszFaceName
    );

    // set our new font to the system
    HFONT prev_font = (HFONT )SelectObject(application::singleton()->window()->hdc(), _handle);

    static bool once = false;
    if (!once) {
        for (int ii = 'A'; ii <= 'Z'; ++ii) {
            foo(application::singleton()->window()->hdc(), ii, square(1.f / 32.f));
        }
        once = true;
    }

    // generate font bitmaps with selected HFONT
    memset( &m, 0, sizeof(m) );
    m.eM11.value = 1;
    m.eM12.value = 0;
    m.eM21.value = 0;
    m.eM22.value = 1;

    for (auto block : unicode_data) {
        wglUseFontBitmapsW(application::singleton()->window()->hdc(), block.from, block.size, _list_base + block.offset);
        for (int ii = 0; ii < block.size; ++ii) {
            GetGlyphOutlineW(application::singleton()->window()->hdc(), block.from + ii, GGO_METRICS, &gm, 0, NULL, &m);
            _char_width[block.offset + ii] = gm.gmCellIncX;
        }
    }

    // restore previous font
    SelectObject(application::singleton()->window()->hdc(), prev_font);
}

//------------------------------------------------------------------------------
font::~font()
{
    // restore system font if this is the active font
    if (_active_font == _handle) {
        glListBase(0);
        SelectObject(application::singleton()->window()->hdc(), _system_font);

        _active_font = _system_font;
        _system_font = NULL;
    }

    // delete from opengl
    if (_list_base) {
        glDeleteLists(_list_base, unicode_data.max_index() + 1);
    }

    // delete font from gdi
    if (_handle) {
        DeleteObject(_handle);
    }
}

//------------------------------------------------------------------------------
bool font::compare(string::view name, int size) const
{
    return _name == name
        && _size == size;
}

//------------------------------------------------------------------------------
void font::draw(string::view string, vec2 position, color4 color, vec2 scale) const
{
    // activate font if it isn't already
    if (_active_font != _handle) {
        HFONT prev_font = (HFONT )SelectObject(application::singleton()->window()->hdc(), _handle);

        // keep track of the system font so it can be restored later
        if (_system_font == NULL) {
            _system_font = prev_font;
        }

        glListBase(_list_base);
        _active_font = _handle;
    }

    int xoffs = 0;

    int r = static_cast<int>(color.r * 255.5f);
    int g = static_cast<int>(color.g * 255.5f);
    int b = static_cast<int>(color.b * 255.5f);
    int a = static_cast<int>(color.a * 255.5f);

    char const* cursor = string.begin();
    char const* end = string.end();

    constexpr int buffer_size = 1024;
    char32_t buffer[buffer_size];

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        glColor4ub(narrow_cast<uint8_t>(r),
                   narrow_cast<uint8_t>(g),
                   narrow_cast<uint8_t>(b),
                   narrow_cast<uint8_t>(a));
        glRasterPos2f(position.x + xoffs * scale.x, position.y);

        while (cursor < next) {
            int n = 0;

            // Decode UTF-8 into codepoints
            while (cursor < next && n < buffer_size) {
                int c = unicode_data.decode(cursor);
                buffer[n++] = c;
            }

            // Rewrite text based on directionality/joining
            char32_t* p = unicode::rewrite(buffer, buffer + n);
            // Contextual conversion may result in fewer codepoints
            n = narrow_cast<GLsizei>(p - buffer);

            // Convert codepoints to character indices
            for (int ii = 0; ii < n; ++ii) {
                buffer[ii] = unicode_data.codepoint_to_index(buffer[ii]);
                xoffs += _char_width[buffer[ii]];
            }

            glCallLists(n, GL_INT, buffer);
        }

        if (cursor < end && is_color(cursor)) {
            if (!get_color(cursor, r, g, b)) {
                r = static_cast<int>(color.r * 255.5f);
                g = static_cast<int>(color.g * 255.5f);
                b = static_cast<int>(color.b * 255.5f);
            }
            cursor += 4;
        }
    }
}

//------------------------------------------------------------------------------
vec2 font::size(string::view string, vec2 scale) const
{
    vec2i size(0, _size);
    char const* cursor = string.begin();
    char const* end = string.end();

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        while (cursor < next) {
            int ch = unicode_data.decode_index(cursor);
            size.x += _char_width[ch];
        }

        if (cursor < end) {
            cursor += 4;
        }
    }

    return vec2(size) * scale;
}

} // namespace render
