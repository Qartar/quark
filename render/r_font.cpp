// r_font.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_filesystem.h"
#include "cm_unicode.h"
#include "gl/gl_include.h"
#include "gl/gl_shader.h"

#include <numeric> // iota

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

    //! Returns a pointer to the beginning of the Unicode block layouts
    constexpr auto data() const { return _blocks.data(); }

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
        { 0xfb50, 0xfdff, "Arabic Presentation Forms-A" },
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
    // bounding box of points
    bounds bounds;

    // glyph metrics provided by GetGlyphOutlineW
    GLYPHMETRICS metrics;
    // outline data provided by GetGlyphOutlineW
    std::vector<std::byte> outline;

    //! offset into outline data to TTPOLYCURVE for each point on a bezier curve
    std::vector<std::ptrdiff_t> qspline_offset;
    //! number of points from this point to the first point not on the bezier curve
    std::vector<std::size_t> qspline_points;
    //! starting point of the bezier curve (which is not stored in the TTPOLYCURVE data)
    std::vector<vec2> qspline_start;
};

//------------------------------------------------------------------------------
struct font_sdf
{
    /*
          cell
      v0 /
        o------------o
        | |          |
        | | origin   |
        | |/         | size.y
        |-x----------|----x
        | |          |    :
        o------------o    :
          : size.x    v1  :
          :               :
          : -- advance -- :

        v0 = p - glyph.offset
        v1 = p - glyph.offset + glyph.size

        uv0 = glyph.cell
        uv1 = glyph.cell + glyph.size
    */
    using glyph_info = font::glyph_info;

    struct block
    {
        char32_t from;
        char32_t to;
        int offset;
    };

    struct image
    {
        std::size_t width;
        std::size_t height;
        std::vector<std::uint32_t> data;
    };

    float line_gap;
    std::vector<block> blocks;
    std::vector<glyph_info> glyphs;
    image image;
    gl::texture2d texture;

    int codepoint_to_glyph_index(char32_t cp) const {
        for (std::size_t block_index = 0; block_index < blocks.size(); ++block_index) {
            if (cp >= blocks[block_index].from && cp <= blocks[block_index].to) {
                return blocks[block_index].offset + (cp - blocks[block_index].from);
            }
        }
        return 0; // FIXME: return glyph index for unicode replacement character
    }
};

glyph rasterize_glyph(HDC hdc, UINT ch, float chordalDeviationSquared = 0.f);

//------------------------------------------------------------------------------
float determinant(vec2 a, vec2 b)
{
    float cd = a.y * b.x;
    float err = std::fma(-a.y, b.x, cd);
    return std::fma(a.x, b.y, -cd) + err;
}

//------------------------------------------------------------------------------
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
// https://www.shadertoy.com/view/MlKcDD
// Copyright © 2018 Inigo Quilez
edge_distance signed_qspline_distance_squared(vec2 A, vec2 B, vec2 C, vec2 pos)
{
    vec2 a = B - A;
    vec2 b = A - 2.f * B + C;
    vec2 c = a * 2.f;
    vec2 d = A - pos;

    float kk = 1.f / dot(b, b);
    float kx = kk * dot(a, b);
    float ky = kk * (2.f * dot(a, a) + dot(d, b)) / 3.f;
    float kz = kk * dot(d, a);

    float res = 0.f;
    float sgn = 0.f;
    float t = 0.f;

    float p = ky - kx * kx;
    float p3 = p * p * p;
    float q = kx * (2.f * kx * kx - 3.f * ky) + kz;
    float h = q * q + 4.f * p3;

    if (h >= 0.f) {
        h = std::sqrt(h);
        vec2 x = (vec2(h, -h) - vec2(q)) / 2.f;
        vec2 uv = vec2(std::cbrt(x.x), std::cbrt(x.y));
        t = clamp(uv.x + uv.y - kx, 0.f, 1.f);
        vec2 qx = d + (c + b * t) * t;
        res = dot(qx, qx);
        sgn = cross(c + 2.f * b * t, qx);
    } else {
        float z = std::sqrt(-p);
        float v = std::acos(q / (p * z * 2.f)) / 3.f;
        float m = std::cos(v);
        float n = std::sin(v) * 1.732050808f; // sqrt(3)

        float tx = clamp((m + m) * z - kx, 0.f, 1.f);
        vec2 qx = d + (c + b * tx) * tx;
        float dx = dot(qx, qx);
        float sx = cross(c + 2.f * b * tx, qx);

        float ty = clamp((-n - m) * z - kx, 0.f, 1.f);
        vec2 qy = d + (c + b * ty) * ty;
        float dy = dot(qy, qy);
        float sy = cross(c + 2.f * b * ty, qy);

        // the third root cannot be the closest
        //float tz = clamp((n - m) * z - kx, 0.f, 1.f);
        //  ...

        res = (dx < dy) ? dx : dy;
        sgn = (dx < dy) ? sx : sy;
        t = (dx < dy) ? tx : ty;
    }

    if (t == 0.f) {
        float den = dot(B - A, B - A);
        float det = determinant(B - A, pos - A);
        return {std::copysign(res, -sgn), det * det / den};
    } else if (t == 1.f) {
        float den = dot(C - B, C - B);
        float det = determinant(C - B, pos - C);
        return {std::copysign(res, -sgn), det * det / den};
    } else {
        return {std::copysign(res, -sgn), res};
    }
}

//------------------------------------------------------------------------------
edge_distance signed_qspline_distance_squared(TTPOLYCURVE const* pc, vec2 p0, vec2 pos)
{
    edge_distance sdsqr_min = {FLT_MAX, -FLT_MAX};

    auto const GetFixedPoint = [](POINTFX const& pfx) {
        return vec2(float(pfx.x.value) + pfx.x.fract * (1.f / 65536.f),
                    float(pfx.y.value) + pfx.y.fract * (1.f / 65536.f));
    };

    assert(pc->wType == TT_PRIM_QSPLINE);

    // cf. MakeLinesFromTTQSpline
    vec2 p2, p1;
    // To ensure C2 continuity the spline is specified as a series of
    // control points corresponding to each curve. The endpoints of
    // each curve are the midpoints between control points plus the
    // final point on the previous curve and an explicit endpoint in
    // the final curve of the spline.
    for (WORD ii = 0; ii < pc->cpfx - 1; ++ii) {
        p1 = GetFixedPoint(pc->apfx[ii]);
        p2 = GetFixedPoint(pc->apfx[ii + 1]);
        // If this is not the final curve with explicit endpoint
        // calculate the endpoint as midpoint between the current
        // and next control points.
        if (ii != pc->cpfx - 2) {
            p2 = .5f * (p1 + p2);
        }
        edge_distance sdsqr = signed_qspline_distance_squared(p0, p1, p2, pos);
        if (sdsqr < sdsqr_min) {
            sdsqr_min = sdsqr;
        }
        // First endpoint of the next curve is the last endpoint
        // of the current curve.
        p0 = p2;
    }

    return sdsqr_min;
}

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
    if (edge_index >= glyph.edges.size()) {
        return sdsqr_min;
    }
    std::size_t first = glyph.edges[edge_index].first;
    std::size_t last = glyph.edges[edge_index].last;
    for (std::size_t ii = first; ii < last; ++ii) {
#if 1
        // Corner detection may insert corners into the middle of a bezier curve.
        // These corners are spurious since the curves are by construction C1
        // continuous. This effectively ignores detected corners by checking the
        // distance along the entire curve regardless of where the 'edge' starts
        // within the curve. In theory this could cause incorrect per-channel
        // distances across corners but since the corners are spurious this
        // doesn't seem to affect the quality resulting SDF.
        if (glyph.qspline_offset[ii]) {
            TTPOLYCURVE const* pc = reinterpret_cast<TTPOLYCURVE const*>(glyph.outline.data() + glyph.qspline_offset[ii]);
            edge_distance sdsqr = signed_qspline_distance_squared(pc, glyph.qspline_start[ii], p);
            if (sdsqr < sdsqr_min) {
                sdsqr_min = sdsqr;
            }
            assert(ii + glyph.qspline_points[ii] >= glyph.qspline_offset.size()
                || glyph.qspline_offset[ii] != glyph.qspline_offset[ii + glyph.qspline_points[ii]]);
            ii += glyph.qspline_points[ii] - 1;
        }
        // Also need to check the segment between the last point on this curve
        // and the first point in the next segment or curve.
        if (ii < last)
#endif
        {
            edge_distance sdsqr = signed_segment_distance_squared(glyph.points[ii], glyph.points[ii + 1], p);
            if (sdsqr < sdsqr_min) {
                sdsqr_min = sdsqr;
            }
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
int signed_intersect_segments(vec2 a, vec2 b, vec2 c, vec2 d)
{
    vec2 ab = b - a;
    vec2 cd = d - c;
    float den = determinant(ab, cd);
    if (den == 0.f) {
        return 0;
    }
    vec2 ac = c - a;
    float sgn = den < 0.f ? -1.f : 1.f;
    den = std::abs(den);
    float u = determinant(ac, cd) * sgn;
    float v = determinant(ac, ab) * sgn;
    if (0.f <= u && u <= den && 0.f <= v && v <= den) {
        return sgn < 0.f ? -1 : 1;
    } else {
        return 0;
    }
}

//------------------------------------------------------------------------------
bool point_inside_glyph(glyph const& glyph, vec2 p)
{
    int n = 0;
    // Count the number of times a segment from the given point to an arbitrary
    // point outside the glyph crosses segments from the glyph. Since the glyph
    // can have overlapping contours it is necessary to track whether the line
    // crosses "into" or "out of" of the contour at each intersection. If the
    // sum is zero then the point is outside of the glyph.
    for (std::size_t ii = 0, sz = glyph.edges.size(); ii < sz; ++ii) {
        for (std::size_t jj = glyph.edges[ii].first; jj < glyph.edges[ii].last; ++jj) {
            n += signed_intersect_segments(p, vec2(-32.f, -16.f), glyph.points[jj], glyph.points[jj + 1]);
        }
    }
    return n == 0;
}

//------------------------------------------------------------------------------
vec3 signed_edge_distance_channels(glyph const& glyph, vec2 p)
{
    edge_distance dsqr_min[3] = {{FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}};
    std::size_t emin[3] = {};

    if (!glyph.edges.size()) {
        return vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    }

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

    if (!glyph.edges.size()) {
        return FLT_MAX;
    }

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
uint32_t pack_r10g10b10a2(color4 color)
{
    uint32_t r = color.r < 0.f ? 0 : color.r > 1.f ? 0x3ff : uint32_t(color.r * 1023.f + .5f);
    uint32_t g = color.g < 0.f ? 0 : color.g > 1.f ? 0x3ff : uint32_t(color.g * 1023.f + .5f);
    uint32_t b = color.b < 0.f ? 0 : color.b > 1.f ? 0x3ff : uint32_t(color.b * 1023.f + .5f);
    uint32_t a = color.a < 0.f ? 0 : color.a > 1.f ? 0x003 : uint32_t(color.a *    3.f + .5f);
    return (a << 30) | (b << 20) | (g << 10) | (r << 0);
}

//------------------------------------------------------------------------------
void generate_bitmap_r10g10b10a2(glyph const& glyph, mat3 image_to_glyph, std::size_t width, std::size_t height, std::size_t row_stride, uint32_t* data)
{
    for (std::size_t yy = 0; yy < height; ++yy) {
        for (std::size_t xx = 0; xx < width; ++xx) {
            // transform pixel center coordinates in image space to glyph space
            vec2 point = vec2(float(xx) + .5f, float(yy) + .5f) * image_to_glyph;
            vec3 d = signed_edge_distance_channels(glyph, point);
            // discretize d and write to bitmap
            color3 c = color3(d * (1.f / 8.f) + vec3(.5f));
            data[yy * row_stride + xx] = pack_r10g10b10a2(color4(c));
        }
    }
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
            data[yy * row_stride + xx * 3 + 0] = clamp<uint8_t>((d[0] + 4.f) * (255.f / 8.f), 0, 255);
            data[yy * row_stride + xx * 3 + 1] = clamp<uint8_t>((d[1] + 4.f) * (255.f / 8.f), 0, 255);
            data[yy * row_stride + xx * 3 + 2] = clamp<uint8_t>((d[2] + 4.f) * (255.f / 8.f), 0, 255);
        }
    }
}

//------------------------------------------------------------------------------
vec3 sample_texel(std::size_t image_stride, uint8_t const* image_data, std::size_t x, std::size_t y)
{
    return vec3(image_data[y * image_stride + x * 3 + 0] * (1.f / 255.f),
                image_data[y * image_stride + x * 3 + 1] * (1.f / 255.f),
                image_data[y * image_stride + x * 3 + 2] * (1.f / 255.f));
}

//------------------------------------------------------------------------------
vec3 sample_bilinear(std::size_t image_width, std::size_t image_height, std::size_t image_stride, uint8_t const* image_data, vec2 p)
{
    // convert normalized texture coordinates to image space, pixel centers
    vec2 v = p * vec2(float(image_width), float(image_height)) - vec2(.5f);
    std::size_t x0 = clamp<std::size_t>(std::floor(v.x), 0, image_width - 2);
    std::size_t y0 = clamp<std::size_t>(std::floor(v.y), 0, image_height - 2);
    float xt = clamp(v.x - x0, 0.f, 1.f); // could be outside [0,1] if x0 is clamped
    float yt = clamp(v.y - y0, 0.f, 1.f); // could be outside [0,1] if y0 is clamped

    vec3 samples[4] = {
        sample_texel(image_stride, image_data, x0 + 0, y0 + 0),
        sample_texel(image_stride, image_data, x0 + 1, y0 + 0),
        sample_texel(image_stride, image_data, x0 + 0, y0 + 1),
        sample_texel(image_stride, image_data, x0 + 1, y0 + 1),
    };

    vec3 sx0 = samples[0] + (samples[1] - samples[0]) * xt;
    vec3 sx1 = samples[2] + (samples[3] - samples[2]) * xt;
    return sx0 + (sx1 - sx0) * yt;
}

//------------------------------------------------------------------------------
void generate_test_bitmap(std::size_t in_width, std::size_t in_height, std::size_t in_stride, uint8_t const* in_data,
                          std::size_t out_width, std::size_t out_height, std::size_t out_stride, uint8_t* out_data)
{
    float s[2][2] = {};

    for (std::size_t yy = 0; yy < out_height; ++yy) {
        for (std::size_t xx = 0; xx < out_width; ++xx) {
            if ((xx & 1) == 0) { // emulate GPU quad shading
                vec2 p00 = vec2(float(xx +  .5f) / float(out_width),
                                float((yy & ~1) +  .5f) / float(out_height));
                vec2 p01 = vec2(float(xx +  .5f) / float(out_width),
                                float((yy & ~1) + 1.5f) / float(out_height));

                vec2 p10 = vec2(float(xx + 1.5f) / float(out_width),
                                float((yy & ~1) +  .5f) / float(out_height));
                vec2 p11 = vec2(float(xx + 1.5f) / float(out_width),
                                float((yy & ~1) + 1.5f) / float(out_height));

                s[0][0] = median(sample_bilinear(in_width, in_height, in_stride, in_data, p00));
                s[0][1] = median(sample_bilinear(in_width, in_height, in_stride, in_data, p01));
                s[1][0] = median(sample_bilinear(in_width, in_height, in_stride, in_data, p10));
                s[1][1] = median(sample_bilinear(in_width, in_height, in_stride, in_data, p11));
            }

            float f = s[xx & 1][yy & 1];
            float fwidth = std::abs(s[(xx & 1) ^ 1][yy & 1] - f) + std::abs(s[xx & 1][(yy & 1) ^ 1] - f);
            float d = clamp((f - .5f) / fwidth + .5f, 0.f, 1.f);

            out_data[yy * out_stride + xx * 3 + 0] = clamp<uint8_t>(d * (255.f / 1.f), 0, 255);
            out_data[yy * out_stride + xx * 3 + 1] = clamp<uint8_t>(d * (255.f / 1.f), 0, 255);
            out_data[yy * out_stride + xx * 3 + 2] = clamp<uint8_t>(d * (255.f / 1.f), 0, 255);
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
            int m = clamp(median(d) * (-4096.f / 1.f), 0, 255);
            data[yy * row_stride + xx * 3 + 0] = clamp<uint8_t>(m + data[yy * row_stride + xx * 3 + 0], 0, 255);
            data[yy * row_stride + xx * 3 + 1] = clamp<uint8_t>(m + data[yy * row_stride + xx * 3 + 1], 0, 255);
            data[yy * row_stride + xx * 3 + 2] = clamp<uint8_t>(m + data[yy * row_stride + xx * 3 + 2], 0, 255);
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
struct dds_pixelformat {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};
struct dds_header {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    dds_pixelformat ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};
struct dds_header_dx10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlag2;
};

static_assert(sizeof(dds_header) == 124);
static_assert(sizeof(dds_pixelformat) == 32);

constexpr uint32_t DDS_MAGIC = ('D' << 0 | 'D' << 8 | 'S' << 16 | ' ' << 24);
constexpr int DDSD_CAPS = 0x1;
constexpr int DDSD_HEIGHT = 0x2;
constexpr int DDSD_WIDTH = 0x4;
constexpr int DDSD_PITCH = 0x8;
constexpr int DDSD_PIXELFORMAT = 0x1000;
constexpr int DDPF_ALPHAPIXELS = 0x1;
constexpr int DDPF_FOURCC = 0x4;
constexpr int DDPF_RGB = 0x40;
constexpr uint32_t FOURCC_DX10 = ('D' << 0 | 'X' << 8 | '1' << 16 | '0' << 24);
constexpr int DDSCAPS_TEXTURE = 0x1000;
constexpr int DXGI_FORMAT_R10G10B10A2_UNORM = 24;
constexpr int DXGI_FORMAT_R8G8B8A8_UNORM = 28;
constexpr int DDS_DIMENSION_TEXTURE2D  = 3;

//------------------------------------------------------------------------------
void write_dds(string::view filename, std::size_t image_width, std::size_t image_height, std::vector<uint32_t> const& image_data, bool legacy)
{
    dds_header header{};
    header.dwSize = sizeof(dds_header);
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
    header.dwHeight = narrow_cast<uint32_t>(image_height);
    header.dwWidth = narrow_cast<uint32_t>(image_width);
    header.dwDepth = 1; // not required
    header.dwMipMapCount = 1; // not required
    header.dwPitchOrLinearSize = narrow_cast<uint32_t>(image_width * sizeof(uint32_t));
    header.ddspf.dwSize = sizeof(dds_pixelformat);
    header.dwCaps = DDSCAPS_TEXTURE;

    // Legacy encoding for tools that don't support DDS_HEADER_DX10
    if (legacy) {
        header.ddspf.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
        header.ddspf.dwRGBBitCount = 32;
        header.ddspf.dwRBitMask = 0x000003ff;
        header.ddspf.dwGBitMask = 0x000ffc00;
        header.ddspf.dwBBitMask = 0x3ff00000;
        header.ddspf.dwABitMask = 0xc0000000;

        auto f = file::open(filename, file::mode::write);
        f.write((file::byte const*)&DDS_MAGIC, sizeof(DDS_MAGIC));
        f.write((file::byte const*)&header, sizeof(header));
        f.write((file::byte const*)image_data.data(), image_data.size() * sizeof(uint32_t));
        f.close();
    } else {
        header.ddspf.dwFlags = DDPF_FOURCC;
        header.ddspf.dwFourCC = FOURCC_DX10;

        dds_header_dx10 header10{};
        header10.dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
        header10.resourceDimension = DDS_DIMENSION_TEXTURE2D;

        auto f = file::open(filename, file::mode::write);
        f.write((file::byte const*)&DDS_MAGIC, sizeof(DDS_MAGIC));
        f.write((file::byte const*)&header, sizeof(header));
        f.write((file::byte const*)&header10, sizeof(header10));
        f.write((file::byte const*)image_data.data(), image_data.size() * sizeof(uint32_t));
        f.close();
    }
}

//------------------------------------------------------------------------------
bool read_dds(string::view filename, std::size_t& image_width, std::size_t& image_height, std::vector<uint32_t>& image_data)
{
    auto f = file::read(filename);
    if (f.size() < sizeof(uint32_t) + sizeof(dds_header)
        || *reinterpret_cast<uint32_t const*>(f.data()) != DDS_MAGIC) {
        return false;
    }

    dds_header const* header = reinterpret_cast<dds_header const*>(f.data() + 4);
    if (header->ddspf.dwFlags & DDPF_FOURCC && header->ddspf.dwFourCC == FOURCC_DX10) {
        if (f.size() < sizeof(uint32_t) + sizeof(dds_header) + sizeof(dds_header_dx10)) {
            return false;
        }
        dds_header_dx10 const* header10 = reinterpret_cast<dds_header_dx10 const*>(&header[1]);
        if (header10->dxgiFormat != DXGI_FORMAT_R10G10B10A2_UNORM
            || header10->resourceDimension != DDS_DIMENSION_TEXTURE2D) {
            return false;
        }
        image_width = header->dwWidth;
        image_height = header->dwHeight;
        image_data.resize(image_width * image_height);
        memcpy(image_data.data(), reinterpret_cast<std::byte const*>(&header10[1]), image_data.size() * sizeof(uint32_t));
        return true;
    } else {
        if (header->ddspf.dwFlags != (DDPF_ALPHAPIXELS | DDPF_RGB)
            || header->ddspf.dwRGBBitCount != 32
            || header->ddspf.dwRBitMask != 0x000003ff
            || header->ddspf.dwGBitMask != 0x000ffc00
            || header->ddspf.dwBBitMask != 0x3ff00000
            || header->ddspf.dwABitMask != 0xc0000000) {
            return false;
        }
        image_width = header->dwWidth;
        image_height = header->dwHeight;
        image_data.resize(image_width * image_height);
        memcpy(image_data.data(), reinterpret_cast<std::byte const*>(&header[1]), image_data.size() * sizeof(uint32_t));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
void write_sdf(string::view filename, font_sdf const* sdf)
{
    auto f = file::open(filename, file::mode::write);
    int num_blocks = narrow_cast<int>(sdf->blocks.size());
    f.write((file::byte const*)&num_blocks, sizeof(num_blocks));
    for (int ii = 0; ii < num_blocks; ++ii) {
        f.write((file::byte const*)&sdf->blocks[ii].from, sizeof(sdf->blocks[ii].from));
        f.write((file::byte const*)&sdf->blocks[ii].to, sizeof(sdf->blocks[ii].to));
    }
    f.write((file::byte const*)sdf->glyphs.data(), sdf->glyphs.size() * sizeof(font_sdf::glyph_info));
}

//------------------------------------------------------------------------------
bool read_sdf(string::view filename, font_sdf* sdf)
{
    auto f = file::open(filename, file::mode::read);
    if (!f) {
        return false;
    }
    int num_blocks = 0; f.read((file::byte*)&num_blocks, sizeof(num_blocks));
    sdf->blocks.resize(num_blocks);
    int num_glyphs = 0;
    for (int ii = 0; ii < num_blocks; ++ii) {
        f.read((file::byte*)&sdf->blocks[ii].from, sizeof(sdf->blocks[ii].from));
        f.read((file::byte*)&sdf->blocks[ii].to, sizeof(sdf->blocks[ii].to));
        sdf->blocks[ii].offset = num_glyphs;
        num_glyphs += sdf->blocks[ii].to + 1 - sdf->blocks[ii].from;
    }
    sdf->glyphs.resize(num_glyphs);
    f.read((file::byte*)sdf->glyphs.data(), sdf->glyphs.size() * sizeof(font_sdf::glyph_info));
    return true;
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
int pack_glyphs(int width, std::size_t num_glyphs, vec2i const* sizes, vec2i* offsets)
{
    std::vector<std::size_t> index(num_glyphs);
    std::iota(index.begin(), index.end(), 0);
    // sort descending by height, i.e.
    //      sizes[index[N]].y >= sizes[index[N+1]].y
    std::sort(index.begin(), index.end(), [sizes](std::size_t lhs, std::size_t rhs) {
        return (sizes[lhs].y > sizes[rhs].y) || (sizes[lhs].y == sizes[rhs].y && sizes[lhs].x > sizes[rhs].x);
    });

    int pack_height = 0;
    std::deque<vec2i> prev_steps = {vec2i(0, 0), vec2i(width, 0)};
    std::size_t step_index = 0;
    std::deque<vec2i> row_steps;
    bool left_to_right = true;
    int x = 0;

    for (std::size_t ii = 0; ii < num_glyphs; ++ii) {
        // size of current glyph
        std::size_t idx = index[ii];
        vec2i sz = sizes[idx];

        // start a new row
        if ((left_to_right && x + sz.x > width) || (!left_to_right && x - sz.x < 0)) {
            // failed to place any glyphs on the current row, width is too small
            if (!row_steps.size()) {
                return 0;
            }
            std::swap(row_steps, prev_steps);
            row_steps.clear();
            // extend steps to full width
            prev_steps.front().x = 0;
            prev_steps.back().x = width;

            // determine left_to_right
            left_to_right = prev_steps.front().y < prev_steps.back().y;
            x = left_to_right ? 0 : width;
            step_index = left_to_right ? 0 : prev_steps.size() - 2;
        }

        // place glyph

        if (left_to_right) {
            // assume fits on the current step
            offsets[idx] = vec2i(x, prev_steps[step_index].y);

            std::size_t next_index = step_index;
            while (x + sz.x > prev_steps[next_index + 1].x) {
                next_index += 2;
                assert(next_index < prev_steps.size());
            }
            // check if the glyph needs to be bumped down
            if (prev_steps[next_index].y > prev_steps[step_index].y) {
                offsets[idx].y += prev_steps[next_index].y - prev_steps[step_index].y;
            }
            // row_steps are always left-to-right
            row_steps.push_back(offsets[idx] + vec2i(0, sz.y));
            row_steps.push_back(offsets[idx] + sz);
            pack_height = max(pack_height, row_steps.back().y);
            x += sz.x;
            // advance step
            step_index = next_index;
        } else {
            // assume fits on the current step
            offsets[idx] = vec2i(x - sz.x, prev_steps[step_index + 1].y);

            std::size_t next_index = step_index;
            while (x - sz.x < prev_steps[next_index].x) {
                next_index -= 2;
                assert(next_index < prev_steps.size()); // i.e. next_index >= 0 via underflow
            }
            // check if the glyph needs to be bumped down
            if (prev_steps[next_index + 1].y > prev_steps[step_index + 1].y) {
                offsets[idx].y += prev_steps[next_index + 1].y - prev_steps[step_index + 1].y;
            }
            // row_steps are always left-to-right
            row_steps.push_front(offsets[idx] + sz);
            row_steps.push_front(offsets[idx] + vec2i(0, sz.y));
            pack_height = max(pack_height, row_steps.front().y);
            x -= sz.x;
            // advance step
            step_index = next_index;
        }
    }

    return pack_height;
}

//------------------------------------------------------------------------------
void test_font_sdf(font_sdf const& sdf, char const* str, int image_width, int image_height, int scale)
{
#if 0
    std::vector<uint8_t> image_data(image_width * image_height * 3);
    int cx = image_width + 8;
    int cy = image_height / 2;
    int row_stride = image_width * 3;

    for (char const* p = str; *p; ++p) {
        font_sdf::glyph const& g = sdf.blocks[0].glyphs[int(*p) - sdf.blocks[0].from];

        int x0 = cx + int(g.offset.x * scale);
        int y0 = cy + int(g.offset.y * scale);
        float u0 = (g.cell.x + .5f) / float(sdf.image.width);
        float v0 = (g.cell.y + .5f) / float(sdf.image.height);
        float du = g.size.x / float(sdf.image.width) / (g.size.x * scale);
        float dv = g.size.y / float(sdf.image.height) / (g.size.y * scale);

        float s[2][2] = {};

        for (int yy = 0; yy < g.size.y * scale; ++yy) {
            for (int xx = 0; xx < g.size.x * scale; ++xx) {
#if 1
                if ((xx & 1) == 0) { // emulate GPU quad shading
                    vec2 p00 = vec2(u0 + du * float(xx),
                                    v0 + dv * float(yy & ~1));
                    vec2 p01 = vec2(u0 + du * float(xx),
                                    v0 + dv * float((yy & ~1) + 1));
                    vec2 p10 = vec2(u0 + du * float(xx + 1),
                                    v0 + dv * float(yy & ~1));
                    vec2 p11 = vec2(u0 + du * float(xx + 1),
                                    v0 + dv * float((yy & ~1) + 1));

                    s[0][0] = median(sample_bilinear(sdf.image.width, sdf.image.height, sdf.image.width * 3, sdf.image.data.data(), p00));
                    s[0][1] = median(sample_bilinear(sdf.image.width, sdf.image.height, sdf.image.width * 3, sdf.image.data.data(), p01));
                    s[1][0] = median(sample_bilinear(sdf.image.width, sdf.image.height, sdf.image.width * 3, sdf.image.data.data(), p10));
                    s[1][1] = median(sample_bilinear(sdf.image.width, sdf.image.height, sdf.image.width * 3, sdf.image.data.data(), p11));
                }

                float f = s[xx & 1][yy & 1];
                float fwidth = std::abs(s[(xx & 1) ^ 1][yy & 1] - f) + std::abs(s[xx & 1][(yy & 1) ^ 1] - f);
                float d = clamp((.5f - f) / fwidth + .5f, 0.f, 1.f);

                image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 0] = clamp<uint8_t>(d * (255.f / 1.f) + image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 0], 0, 255);
                image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 1] = clamp<uint8_t>(d * (255.f / 1.f) + image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 1], 0, 255);
                image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 2] = clamp<uint8_t>(d * (255.f / 1.f) + image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 2], 0, 255);
#else
                (void)s;

                vec2 uv = vec2(u0 + du * float(xx), v0 + dv * float(yy));
                vec3 px = sample_bilinear(sdf.image.width, sdf.image.height, sdf.image.width * 3, sdf.image.data.data(), uv);

                image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 0] = clamp<uint8_t>(px[0] * (255.f / 1.f) + image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 0], 0, 255);
                image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 1] = clamp<uint8_t>(px[1] * (255.f / 1.f) + image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 1], 0, 255);
                image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 2] = clamp<uint8_t>(px[2] * (255.f / 1.f) + image_data[(y0 + yy) * row_stride + (x0 + xx) * 3 + 2], 0, 255);
#endif
            }
        }

        cx += int(g.advance) * scale;
    }

    write_bitmap("pack/text.bmp", image_width, image_height, image_data);
#else
    (void)sdf;
    (void)str;
    (void)image_width;
    (void)image_height;
    (void)scale;
#endif
}

//------------------------------------------------------------------------------
void test_signed_qspline_distance_squared()
{
    int image_width = 256;
    int image_height = 256;
    std::vector<uint8_t> image_data(image_width * image_height * 3);

    vec2 a = vec2(128.f, 32.f);
    vec2 b = vec2(192.f, 192.f);
    vec2 c = vec2(32.f, 128.f);

    for (int jj = 0; jj < image_height; ++jj) {
        for (int ii = 0; ii < image_width; ++ii) {
            vec2 p = vec2(float(ii) + .5f, float(jj) + .5f);
            float d = signed_qspline_distance_squared(a, b, c, p).signed_distance_sqr;

            uint8_t m = uint8_t(clamp<int>(int(std::sqrt(std::abs(d)) + .5f), 0, 255));
            image_data[(jj * image_width + ii) * 3 + 0] = d > 0.f ? 0 : m;
            image_data[(jj * image_width + ii) * 3 + 1] = m;
            image_data[(jj * image_width + ii) * 3 + 2] = d > 0.f ? m : 0;
        }
    }

    write_bitmap("sdf/qspline.bmp", image_width, image_height, image_data);
}

//------------------------------------------------------------------------------
void test_pack_glyphs(HDC hdc, unicode_block_layout const* blocks, std::size_t num_blocks, int width, font_sdf* sdf)
{
    constexpr int scale = 1;
    constexpr int margin = 2;

    int total_height = 0;
    std::vector<uint32_t> image_data;

    for (std::size_t block_index = 0; block_index < num_blocks; ++block_index) {

    std::vector<vec2i> sizes;
    std::vector<glyph> glyphs;
    std::vector<int> cell_index;
    sizes.reserve(blocks[block_index].size);
    glyphs.reserve(blocks[block_index].size);
    cell_index.reserve(blocks[block_index].size);

    MAT2 mat = {
        {0, 1}, {0, 0},
        {0, 0}, {0, 1},
    };

    for (int ii = 0; ii < blocks[block_index].size; ++ii) {
        glyphs.push_back(rasterize_glyph(hdc, blocks[block_index].from + ii, 0.f));
        cell_index.push_back(narrow_cast<int>(sizes.size()));
        for (int jj = 0; jj < ii; ++jj) {
            if (glyphs[ii].outline.size() != glyphs[jj].outline.size()) {
                continue;
            }
            if (!memcmp(glyphs[ii].outline.data(), glyphs[jj].outline.data(), glyphs[ii].outline.size())) {
                cell_index[ii] = cell_index[jj];
                break;
            }
        }
        if (cell_index.back() == sizes.size()) {
            sizes.push_back({
                int(glyphs[ii].metrics.gmBlackBoxX + margin * 2),
                int(glyphs[ii].metrics.gmBlackBoxY + margin * 2)
            });
        }
    }

    std::vector<vec2i> offsets(sizes.size());

    int pack_height = pack_glyphs(width, sizes.size(), sizes.data(), offsets.data());
    image_data.resize(width * scale * (total_height + pack_height) * scale);
    int row_stride = width * scale;

    font_sdf::block block;
    block.from = blocks[block_index].from;
    block.to = blocks[block_index].to;
    block.offset = narrow_cast<int>(sdf->glyphs.size());

    for (int ii = 0; ii < offsets.size(); ++ii) {
        offsets[ii].y += total_height;
    }

    for (int ii = 0; ii < glyphs.size(); ++ii) {
        int jj = cell_index[ii];
        int x0 = offsets[jj].x * scale;
        int y0 = offsets[jj].y * scale;

        uint32_t* glyph_data = image_data.data() + y0 * row_stride + x0;
        {
            mat3 image_to_glyph = mat3(1.f / float(scale), 0.f, 0.f,
                                       0.f, 1.f / float(scale), 0.f,
                                       float(glyphs[ii].metrics.gmptGlyphOrigin.x - margin),
                                       float(glyphs[ii].metrics.gmptGlyphOrigin.y - sizes[jj].y + margin),
                                       1.f);

            generate_bitmap_r10g10b10a2(glyphs[ii], image_to_glyph, sizes[jj].x * scale, sizes[jj].y * scale, row_stride, glyph_data);
        }

        {
            font_sdf::glyph_info g;
            g.size = vec2((sizes[jj] - vec2i(2)) * scale);
            g.cell = vec2((offsets[jj] + vec2i(1)) * scale);
            g.offset = vec2(float(glyphs[ii].metrics.gmptGlyphOrigin.x) * scale,
                            float(glyphs[ii].metrics.gmptGlyphOrigin.y - (sizes[jj].y - margin)) * scale);
            g.advance = vec2(float(glyphs[ii].metrics.gmCellIncX * scale), 0);
            sdf->glyphs.push_back(g);
        }
    }

    sdf->blocks.push_back(std::move(block));
    total_height += pack_height;
    }

    int height = 1; while (height < total_height) { height <<= 1; }
    image_data.resize(width * scale * height * scale);

    sdf->image.width = width * scale;
    sdf->image.height = height * scale;
    sdf->image.data = image_data;
    sdf->texture = gl::texture2d(1, GL_RGB10_A2, narrow_cast<GLsizei>(sdf->image.width), narrow_cast<GLsizei>(sdf->image.height));
    sdf->texture.upload(0, 0, 0, narrow_cast<GLsizei>(sdf->image.width), narrow_cast<GLsizei>(sdf->image.height), GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, sdf->image.data.data());
}

//------------------------------------------------------------------------------
glyph rasterize_glyph(HDC hdc, UINT ch, float chordalDeviationSquared)
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
    MAT2 mat = {
        {0, 1}, {0, 0},
        {0, 0}, {0, 1},
    };

    DWORD glyphBufSize = GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &g.metrics, 0, NULL, &mat);
    g.outline.resize(glyphBufSize);
    std::vector<vec2> points;

    std::vector<std::ptrdiff_t> qspline_offset;
    std::vector<std::size_t> qspline_points;
    std::vector<vec2> qspline_start;

    GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &g.metrics, glyphBufSize, g.outline.data(), &mat);

    // cf. MakeLinesFromGlyph
    TTPOLYGONHEADER const* ph = reinterpret_cast<TTPOLYGONHEADER const*>(g.outline.data());
    TTPOLYGONHEADER const* hend = reinterpret_cast<TTPOLYGONHEADER const*>(g.outline.data() + glyphBufSize);
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
        qspline_offset.push_back(0);
        qspline_points.push_back(0);
        qspline_start.push_back(vec2_zero);

        TTPOLYCURVE const* pc = reinterpret_cast<TTPOLYCURVE const*>(&ph[1]);
        TTPOLYCURVE const* cend = reinterpret_cast<TTPOLYCURVE const*>(reinterpret_cast<std::byte const*>(ph) + ph->cb);

        while (pc < cend) {
            // cf. MakeLinesFromTTPolycurve
            if (pc->wType == TT_PRIM_LINE) {
                // cf. MakeLinesFromTTLine
                for (WORD ii = 0; ii < pc->cpfx; ++ii) {
                    points.push_back(GetFixedPoint(pc->apfx[ii]));
                    qspline_offset.push_back(0);
                    qspline_points.push_back(0);
                    qspline_start.push_back(vec2_zero);
                }
            } else if (pc->wType == TT_PRIM_QSPLINE) {
                std::size_t count = points.size();

                // cf. MakeLinesFromTTQSpline
                vec2 p2, p1, p0 = points.back();
                // To ensure C2 continuity the spline is specified as a series of
                // control points corresponding to each curve. The endpoints of
                // each curve are the midpoints between control points plus the
                // final point on the previous curve and an explicit endpoint in
                // the final curve of the spline.
                for (WORD ii = 0; ii < pc->cpfx - 1; ++ii) {
                    p1 = GetFixedPoint(pc->apfx[ii]);
                    p2 = GetFixedPoint(pc->apfx[ii + 1]);
                    // If this is not the final curve with explicit endpoint
                    // calculate the endpoint as midpoint between the current
                    // and next control points.
                    if (ii != pc->cpfx - 2) {
                        p2 = .5f * (p1 + p2);
                    }
                    subdivide_qspline(points, p0, p1, p2, chordalDeviationSquared);
                    // First endpoint of the next curve is the last endpoint
                    // of the current curve.
                    p0 = p2;
                }

                points.push_back(p2);

                while (qspline_offset.size() < points.size()) {
                    qspline_offset.push_back((std::byte const*)pc - g.outline.data());
                    qspline_points.push_back(points.size() - qspline_points.size());
                    qspline_start.push_back(points[count - 1]);
                }

            } else if (pc->wType == TT_PRIM_CSPLINE) {
                // Cubic B-splines are only returned from GetGlyphOutline with GGO_BEZIER
                return {};
            } else {
                return {};
            }

            pc = reinterpret_cast<TTPOLYCURVE const*>(&pc->apfx[pc->cpfx]);
        }

#if 0
        for (std::size_t ii = pstart; ii < points.size(); ++ii) {
            OutputDebugStringA(va("%g, %g\n\0", points[ii].x, points[ii].y).begin());
        }
        OutputDebugStringA("\n");
#endif

        if (points[pstart] != points.back()) {
            points.push_back(points[pstart]);
            qspline_offset.push_back(0);
            qspline_points.push_back(0);
            qspline_start.push_back(vec2_zero);
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
#if 0
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
                g.qspline_offset.push_back(qspline_offset[pstart + (pivot - pstart + ii) % pcount]);
                g.qspline_points.push_back(qspline_points[pstart + (pivot - pstart + ii) % pcount]);
                g.qspline_start.push_back(qspline_start[pstart + (pivot - pstart + ii) % pcount]);
            }
            assert(g.points[pstart] == points[pivot]);
            // add rotated start point as duplicate
            g.points.push_back(points[pivot]);
            g.qspline_offset.push_back(0);
            g.qspline_points.push_back(0);
            g.qspline_start.push_back(vec2_zero);
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
                g.qspline_offset.push_back(qspline_offset[pstart + ii]);
                g.qspline_points.push_back(qspline_points[pstart + ii]);
                g.qspline_start.push_back(qspline_start[pstart + ii]);
            }
            // add a single edge containing all points
            g.edge_channels.push_back(masks[g.edges.size() % 3]);
            g.edges.push_back({pstart, pstart + pcount - 1});
        }

        ph = reinterpret_cast<TTPOLYGONHEADER const*>(cend);
    }

#if 0
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

    g.bounds = bounds::from_points(g.points.data(), g.points.size());
    return g;
}

//------------------------------------------------------------------------------
void foo(HDC hdc, UINT ch, float chordalDeviationSquared = 0.f)
{
    glyph g = rasterize_glyph(hdc, ch, chordalDeviationSquared);

    constexpr std::size_t image_width = 32;
    constexpr std::size_t image_height = 32;

    float sx = float(max(g.metrics.gmBlackBoxX + 2, g.metrics.gmBlackBoxY + 2)) / float(image_width);
    float sy = float(max(g.metrics.gmBlackBoxX + 2, g.metrics.gmBlackBoxY + 2)) / float(image_height);

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

    {
        constexpr std::size_t test_width = image_width * 8;
        constexpr std::size_t test_height = image_height * 8;
        std::vector<std::uint8_t> test_data(test_width * test_height * 3);
        generate_test_bitmap(image_width, image_height, image_width * 3, image_data.data(),
                             test_width, test_height, test_width * 3, test_data.data());
        write_bitmap(va("sdf/%s_test.bmp", s), test_width, test_height, test_data);
    }

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
{
#if 0
    GLYPHMETRICS gm;
    MAT2 m;

    // allocate lists
    _list_base = glGenLists(unicode_data.max_index() + 1);
    _char_width.resize(unicode_data.max_index() + 1);

    test_signed_qspline_distance_squared();

    static bool once = true;
    if (!once) {
        for (int ii = 'A'; ii <= 'Z'; ++ii) {
            foo(application::singleton()->window()->hdc(), ii, square(1.f / 32.f));
        }
        for (int ii = 0xfe70; ii <= 0xfeff; ++ii) {
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

    for (auto const& block : unicode_data) {
        wglUseFontBitmapsW(application::singleton()->window()->hdc(), block.from, block.size, _list_base + block.offset);
        for (int ii = 0; ii < block.size; ++ii) {
            GetGlyphOutlineW(application::singleton()->window()->hdc(), block.from + ii, GGO_METRICS, &gm, 0, NULL, &m);
            _char_width[block.offset + ii] = gm.gmCellIncX;
        }
    }
#endif

    _sdf = std::make_unique<font_sdf>();
    // try to load sdf data from file system
    if (read_sdf(va("pack/atlas_%.*s.dat", name.length(), name.begin()), _sdf.get()) &&
        read_dds(va("pack/atlas_%.*s.dds", name.length(), name.begin()), _sdf->image.width, _sdf->image.height, _sdf->image.data)) {

        _sdf->texture = gl::texture2d(1, GL_RGB10_A2, narrow_cast<GLsizei>(_sdf->image.width), narrow_cast<GLsizei>(_sdf->image.height));
        _sdf->texture.upload(0, 0, 0, narrow_cast<GLsizei>(_sdf->image.width), narrow_cast<GLsizei>(_sdf->image.height), GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, _sdf->image.data.data());
    } else {
        // create font
        HFONT font = CreateFontA(
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
        HFONT prev_font = (HFONT )SelectObject(application::singleton()->window()->hdc(), font);

        test_pack_glyphs(application::singleton()->window()->hdc(), unicode_data.data(), unicode_data.size(), 512, _sdf.get());
        write_sdf(va("pack/atlas_%.*s.dat", name.length(), name.begin()), _sdf.get());
        write_dds(va("pack/atlas_%.*s.dds", name.length(), name.begin()), _sdf->image.width, _sdf->image.height, _sdf->image.data, true);

        // restore previous font
        SelectObject(application::singleton()->window()->hdc(), prev_font);
    }

    glGetError();

    if (_sdf.get()) {
        _vao = gl::vertex_array({
            gl::vertex_array_binding{1, {
                gl::vertex_array_attrib{2, GL_FLOAT, gl::vertex_attrib_type::float_, offsetof(instance, position)},
                gl::vertex_array_attrib{1, GL_INT, gl::vertex_attrib_type::integer, offsetof(instance, index)},
                gl::vertex_array_attrib{4, GL_UNSIGNED_BYTE, gl::vertex_attrib_type::normalized, offsetof(instance, color)},
            }}
        });

        _ibo = gl::index_buffer<uint16_t>(gl::buffer_usage::static_, gl::buffer_access::draw, {0, 1, 2, 1, 3, 2});
        _vbo = gl::vertex_buffer<instance>(gl::buffer_usage::stream, gl::buffer_access::draw, 1024);
        _ssbo = gl::shader_storage_buffer<glyph_info>(
            gl::buffer_usage::static_, gl::buffer_access::draw,
            narrow_cast<GLsizei>(_sdf->glyphs.size()), _sdf->glyphs.data());

        _vao.bind_buffer(_ibo);
        _vao.bind_buffer(_vbo, 0);

        gl::index_buffer<int>().bind();
        gl::vertex_buffer<int>().bind();

        string::buffer info_log;
        auto vsh = gl::shader(gl::shader_stage::vertex,
R"(
#version 430 compatibility

struct glyph_info {
    vec2 size;
    vec2 cell;
    vec2 offset;
    vec2 advance;
};

layout(std430, binding = 0) buffer ssbo_glyphs {
    readonly glyph_info glyphs[];
};

layout(location = 0) in vec2 in_position;
layout(location = 1) in int in_index;
layout(location = 2) in vec4 in_color;

out vec2 vtx_texcoord;
out vec4 vtx_color;

void main() {
    vec2 size = glyphs[in_index].size;
    vec2 cell = glyphs[in_index].cell;
    vec2 offset = glyphs[in_index].offset;

    vec2 flip = vec2(1, gl_ProjectionMatrix[1][1] < 0 ? -1 : 1);

    vec4 v0 = vec4(in_position + (offset       ) * flip, 0.0, 1.0);
    vec4 v1 = vec4(in_position + (offset + size) * flip, 0.0, 1.0);
    gl_Position = gl_ModelViewProjectionMatrix * 
                  vec4((gl_VertexID & 1) == 1 ? v1.x : v0.x,
                       (gl_VertexID & 2) == 2 ? v1.y : v0.y, 0.0, 1.0);

    vec2 u0 = cell;
    vec2 u1 = cell + size;
    vtx_texcoord = vec2((gl_VertexID & 1) == 1 ? u1.x : u0.x,
                        (gl_VertexID & 2) == 2 ? u1.y : u0.y);

    vtx_color = in_color;
}
)"
        );

        if (vsh.compile_status(info_log)) {
            if (info_log.length()) {
                log::warning("%s", info_log.c_str());
            }
        } else if (info_log.length()) {
            log::error("%s", info_log.c_str());
        }

        auto fsh = gl::shader(gl::shader_stage::fragment,
R"(
#version 420

layout(binding = 0) uniform sampler2D sdf;

in vec2 vtx_texcoord;
in vec4 vtx_color;

out vec4 frag_color;

float median(vec3 v) {
    float vmin = min(v.x, min(v.y, v.z));
    float vmax = max(v.x, max(v.y, v.z));
    return dot(v, vec3(1.0)) - vmin - vmax;
}

void main() {
    vec2 uv = vtx_texcoord / vec2(textureSize(sdf, 0));
    float f = median(texture(sdf, uv).xyz);
    //float d = clamp((0.5 - f) / fwidth(f), 0.0, 1.0);
    float g = fwidth(f);
    float d = smoothstep(-0.5 * g, 0.5 * g, 0.5 - f);
    frag_color = vec4(1,1,1,d) * vtx_color;
}
)"
        );

        if (fsh.compile_status(info_log)) {
            if (info_log.length()) {
                log::warning("%s", info_log.c_str());
            }
        } else if (info_log.length()) {
            log::error("%s", info_log.c_str());
        }

        _program = gl::program(vsh, fsh);

        if (_program.link_status(info_log)) {
            if (info_log.length()) {
                log::warning("%s", info_log.c_str());
            }
        } else if (info_log.length()) {
            log::error("%s", info_log.c_str());
        }

        if (_program.validate_status(info_log)) {
            if (info_log.length()) {
                log::warning("%s", info_log.c_str());
            }
        } else if (info_log.length()) {
            log::error("%s", info_log.c_str());
        }
    }
}

//------------------------------------------------------------------------------
font::~font()
{
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
    if (!_sdf) {
        return;
    }

    _program.use();
    _vao.bind();
    glEnable(GL_TEXTURE_2D);
    _sdf->texture.bind();
    _ssbo.bind_base(0);

    std::vector<instance> instances;

    using PFNGLDRAWELEMENTSINSTANCED = void (APIENTRY*)(GLenum mode, GLsizei count, GLenum type, void const* indices, GLsizei instancecount);
    auto glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCED)wglGetProcAddress("glDrawElementsInstanced");

    int xoffs = 0;

    int r = static_cast<int>(color.r * 255.f + .5f);
    int g = static_cast<int>(color.g * 255.f + .5f);
    int b = static_cast<int>(color.b * 255.f + .5f);
    int a = static_cast<int>(color.a * 255.f + .5f);

    char const* cursor = string.begin();
    char const* end = string.end();

    constexpr int buffer_size = 1024;
    char32_t buffer[buffer_size];

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        uint32_t packed_color = (a << 24) | (b << 16) | (g << 8) | r;

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
                int glyph_index = _sdf->codepoint_to_glyph_index(buffer[ii]);
                instances.push_back({
                        vec2(float(xoffs), 0),
                        glyph_index,
                        packed_color,
                    });
                xoffs += int(_sdf->glyphs[glyph_index].advance.x);
            }

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glScalef(scale.x, scale.y, 1);
            glTranslatef(position.x / scale.x, position.y / scale.y, 0);

            _vbo.upload(0, instances.size(), instances.data());
            glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr, narrow_cast<GLsizei>(instances.size()));
            instances.resize(0);

            glPopMatrix();
        }

        if (cursor < end && is_color(cursor)) {
            if (!get_color(cursor, r, g, b)) {
                r = static_cast<int>(color.r * 255.f + .5f);
                g = static_cast<int>(color.g * 255.f + .5f);
                b = static_cast<int>(color.b * 255.f + .5f);
            }
            cursor += 4;
        }
    }

    gl::program().use();
    gl::vertex_array().bind();
    glDisable(GL_TEXTURE_2D);
    gl::texture2d().bind();

    gl::index_buffer<int>().bind();
    gl::vertex_buffer<int>().bind();
}

//------------------------------------------------------------------------------
vec2 font::size(string::view string, vec2 scale) const
{
    if (!_sdf) {
        return vec2_zero;
    }

    vec2i size(0, _size);
    char const* cursor = string.begin();
    char const* end = string.end();

    while (cursor < end) {
        char const* next = find_color(cursor, end);
        if (!next) {
            next = end;
        }

        while (cursor < next) {
            int glyph_index = _sdf->codepoint_to_glyph_index(unicode_data.decode(cursor));
            size.x += int(_sdf->glyphs[glyph_index].advance.x);
        }

        if (cursor < end) {
            cursor += 4;
        }
    }

    return vec2(size) * scale;
}

} // namespace render
