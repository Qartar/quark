// r_glyph.cpp
//

#include "precompiled.h"

#include "r_glyph.h"

#include <cfloat> // FLT_MAX

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
glyph glyph::from_hdc(HDC hdc, UINT ch)
{
    glyph g;

    MAT2 const mat = {
        {0, 1}, {0, 0},
        {0, 0}, {0, 1},
    };

    auto const GetFixedPoint = [](POINTFX const& pfx) {
        return vec2(float(pfx.x.value) + pfx.x.fract * (1.f / 65536.f),
                    float(pfx.y.value) + pfx.y.fract * (1.f / 65536.f));
    };

    DWORD glyphBufSize = GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &g._metrics, 0, NULL, &mat);
    g._outline.resize(glyphBufSize);

    GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &g._metrics, glyphBufSize, g._outline.data(), &mat);

    TTPOLYGONHEADER const* ph = reinterpret_cast<TTPOLYGONHEADER const*>(g._outline.data());
    TTPOLYGONHEADER const* hend = reinterpret_cast<TTPOLYGONHEADER const*>(g._outline.data() + glyphBufSize);

    while (ph < hend) {

        if (ph->dwType != TT_POLYGON_TYPE) {
            return {};
        }

        std::size_t pstart = g._points.size();
        std::size_t primstart = g._primitives.size();
        g._points.push_back(GetFixedPoint(ph->pfxStart));

        TTPOLYCURVE const* pc = reinterpret_cast<TTPOLYCURVE const*>(&ph[1]);
        TTPOLYCURVE const* cend = reinterpret_cast<TTPOLYCURVE const*>(reinterpret_cast<std::byte const*>(ph) + ph->cb);

        while (pc < cend) {
            if (pc->wType == TT_PRIM_LINE) {
                // Convert each pair of consecutive points into a unique segment
                for (WORD ii = 0; ii < pc->cpfx; ++ii) {
                    g._primitives.push_back({
                        glyph::primitive_type::segment,
                        g._points.size() - 1,
                        g._points.size()
                    });
                    g._points.push_back(GetFixedPoint(pc->apfx[ii]));
                }
            } else if (pc->wType == TT_PRIM_QSPLINE) {
                // Copy all intermediate control points for the quadratic B-spline
                g._primitives.push_back({
                    glyph::primitive_type::qspline,
                    g._points.size() - 1,
                    g._points.size() - 1 + pc->cpfx
                });
                for (WORD ii = 0; ii < pc->cpfx; ++ii) {
                    g._points.push_back(GetFixedPoint(pc->apfx[ii]));
                }
            } else if (pc->wType == TT_PRIM_CSPLINE) {
                // Cubic B-splines are only returned from GetGlyphOutline with GGO_BEZIER
                return {};
            } else {
                return {};
            }
            // Advance to next poly curve, i.e. primitive
            pc = reinterpret_cast<TTPOLYCURVE const*>(&pc->apfx[pc->cpfx]);
        }
        // Advance to next polygon header, i.e. contour
        ph = reinterpret_cast<TTPOLYGONHEADER const*>(cend);

        // Add implicit final segment if necessary
        if (g._points.back() != g._points[pstart]) {
            g._primitives.push_back({
                glyph::primitive_type::segment,
                g._points.size() - 1,
                g._points.size()
            });
            g._points.push_back(g._points[pstart]);
        }

        //
        // Corner detection
        //

        // Index of the second of two consecutive primitives that form a sharp corner
        std::vector<std::size_t> corners;
        constexpr float cos_15 = 0.96592582628f;

        std::size_t primcount = g._primitives.size() - primstart;
        for (std::size_t ii = primstart, sz = g._primitives.size(); ii < sz; ++ii) {
            std::size_t jj = (ii - primstart + 1) % primcount + primstart;
            assert(g._primitives[ii].last - g._primitives[ii].first >= 1);
            assert(g._primitives[jj].last - g._primitives[jj].first >= 1);

            // The direction between the first or final two points defines the initial and final
            // tangent vector respectively for both line segments and quadradic bezier splines.
            vec2 t0 = g._points[g._primitives[ii].last] - g._points[g._primitives[ii].last - 1];
            vec2 t1 = g._points[g._primitives[jj].first + 1] - g._points[g._primitives[jj].first];

            if (dot(normalize(t0), normalize(t1)) < cos_15) {
                corners.push_back(jj);
            }
        }

        // Rotate primitive ordering so that the first primitive is the start of an
        // edge, this avoids wraparound logic when iterating over edge primitives.
        if (corners.size() && corners.back() != primstart) {
            std::rotate(g._primitives.begin() + primstart, g._primitives.begin() + corners[0], g._primitives.end());
            for (std::size_t ii = corners.size(); ii > 0; --ii) {
                corners[ii - 1] -= (corners[0] - primstart);
            }
        }

        // If there is only one corner the edge needs to be subdivided. For example,
        // a teardrop shape has exactly one sharp corner which needs to have a different
        // channel mask on either side of the corner to preserve sharpness.
        if (corners.size() == 1) {
            // If the other corners are smooth then it will not cause any artifacts to
            // insert an arbitrary break so choose the primitive furthest (by index)
            // from the actual corner.
            corners.push_back((corners.back() + primcount / 2 - primstart) % primcount + primstart);
        }

        //
        // Edge partitioning
        //

        constexpr uint8_t masks[3] = { 0b011, 0b101, 0b110 };

        std::size_t edgestart = g._edges.size();
        for (std::size_t ii = 0, sz = corners.size(); ii < sz; ++ii) {
            // get the index of the preceding primitive of the following corner
            std::size_t jj = (corners[(ii + 1) % sz] + primcount - primstart - 1) % primcount + primstart;
            g._edges.push_back({corners[ii], jj});
            g._edge_channels.push_back(masks[(g._edges.size() - 1) % 3]);
        }

        // If there are no corners create a single edge from all primitives
        if (!corners.size()) {
            g._edges.push_back({primstart, g._primitives.size() - 1});
            g._edge_channels.push_back(masks[(g._edges.size() - 1) % 3]);
        }

        // Make sure first and last edge are not using the same channel, eg. [ABCA] -> [ABCB]
        if (g._edge_channels[edgestart] == g._edge_channels.back()) {
            g._edge_channels.back() = masks[g._edges.size() % 3];
        }

        g._contours.push_back({edgestart, g._edges.size() - 1});
    }

    // Calculate glyph bounds from endpoints of the segments and bezier splines
    // and the intermediate control points of the bezier curves. Since a bezier
    // curve is always contained inside the convex hull of its control points
    // these bounds are guaranteed to contain the entire curve.
    g._bounds = bounds::from_points(g._points.data(), g._points.size());
    return g;
}

//------------------------------------------------------------------------------
float determinant(vec2 a, vec2 b)
{
    float cd = a.y * b.x;
    float err = std::fma(-a.y, b.x, cd);
    return std::fma(a.x, b.y, -cd) + err;
}

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
edge_distance signed_qspline_distance_squared(vec2 const* first, vec2 const* last, vec2 pos)
{
    edge_distance sdsqr_min = {FLT_MAX, -FLT_MAX};

    // To ensure C2 continuity the spline is specified as a series of
    // control points corresponding to each curve. The endpoints of
    // each curve are the midpoints between control points plus the
    // final point on the previous curve and an explicit endpoint in
    // the final curve of the spline.
    vec2 p0 = *first;
    for (vec2 const* points = first + 1; points < last; ++points) {
        vec2 p1 = points[0];
        vec2 p2 = points[1];
        // If this is not the final curve with explicit endpoint
        // calculate the endpoint as midpoint between the current
        // and next control points.
        if (points + 1 < last) {
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
int signed_intersect_qspline(vec2 a, vec2 b, vec2 p0, vec2 p1, vec2 p2)
{
    // A Bezier curve is fully contained in the convex hull of its control
    // points. By testing each edge of the convex hull against the segment
    // we can avoid solving the segment/curve intersection analytically since
    // we don't need the intersection point, just the orientation.
    int i0 = signed_intersect_segments(a, b, p0, p2);
    int i1 = signed_intersect_segments(a, b, p0, p1);
    int i2 = signed_intersect_segments(a, b, p1, p2);

    // Segment does not intersect any edges of the convex region. Assuming
    // the segment is too large to be fully contained in the convex region
    // this means that the curve cannot intersect the segment.
    if (!(i0 || i1 || i2)) {
        return 0;

    // If the segment passes through the convex region then the segment must
    // intersect with the curve in the same orientation.
    } else if (i0 && (i0 == i1 || i0 == i2)) {
        return i0;

    // If the subdivision is too small then just use the intersection with the
    // subdivided curve segments.
    } else if ((p2 - p0).length_sqr() < 1e-6f) {
        return i1 + i2;

    // Otherwise subdivide the curve and recurse.
    } else {
        // de Casteljau's algorithm
        vec2 p01 = .5f * (p0 + p1);
        vec2 p12 = .5f * (p1 + p2);
        vec2 mid = .5f * (p01 + p12); // on the spline

        // Sum the results to account for multiple intersections.
        return signed_intersect_qspline(a, b, p0, p01, mid)
             + signed_intersect_qspline(a, b, mid, p12, p2);
    }
}

//------------------------------------------------------------------------------
int signed_intersect_qspline(vec2 a, vec2 b, vec2 const* first, vec2 const* last)
{
    int n = 0;
    // To ensure C2 continuity the spline is specified as a series of
    // control points corresponding to each curve. The endpoints of
    // each curve are the midpoints between control points plus the
    // final point on the previous curve and an explicit endpoint in
    // the final curve of the spline.
    vec2 p0 = *first;
    for (vec2 const* points = first + 1; points < last; ++points) {
        vec2 p1 = points[0];
        vec2 p2 = points[1];
        // If this is not the final curve with explicit endpoint
        // calculate the endpoint as midpoint between the current
        // and next control points.
        if (points + 1 < last) {
            p2 = .5f * (p1 + p2);
        }
        n += signed_intersect_qspline(a, b, p0, p1, p2);
        // First endpoint of the next curve is the last endpoint
        // of the current curve.
        p0 = p2;
    }

    return n;
}

//------------------------------------------------------------------------------
edge_distance glyph::signed_primitive_distance_squared(vec2 p, std::size_t primitive_index) const
{
    edge_distance sdsqr_min = {FLT_MAX, -FLT_MAX};
    if (primitive_index >= _primitives.size()) {
        return sdsqr_min;
    }
    std::size_t first = _primitives[primitive_index].first;
    std::size_t last = _primitives[primitive_index].last;

    switch (_primitives[primitive_index].type) {
        case glyph::primitive_type::segment:
            return signed_segment_distance_squared(_points[first], _points[last], p);

        case glyph::primitive_type::qspline:
            return signed_qspline_distance_squared(_points.data() + first, _points.data() + last, p);

        case glyph::primitive_type::cspline:
        default:
            return {FLT_MAX, -FLT_MAX};
    }
}

//------------------------------------------------------------------------------
edge_distance glyph::signed_edge_distance_squared(vec2 p, std::size_t edge_index) const
{
    edge_distance sdsqr_min = {FLT_MAX, -FLT_MAX};
    if (edge_index >= _edges.size()) {
        return sdsqr_min;
    }
    std::size_t first = _edges[edge_index].first;
    std::size_t last = _edges[edge_index].last;

    for (std::size_t ii = first; ii <= last; ++ii) {
        edge_distance sdsqr = signed_primitive_distance_squared(p, ii);
        if (sdsqr < sdsqr_min) {
            sdsqr_min = sdsqr;
        }
    }
    return sdsqr_min;
}

//------------------------------------------------------------------------------
// signed edge distance except the edge segments at endpoints are extruded to infinity
float glyph::signed_edge_pseudo_distance(vec2 p, std::size_t edge_index) const
{
    edge_distance sdsqr_min = signed_edge_distance_squared(p, edge_index);

    std::size_t first = _edges[edge_index].first;
    std::size_t last = _edges[edge_index].last;

    std::size_t pfirst = _primitives[first].first;
    std::size_t plast = _primitives[last].last;

    // only compare extruded edge segments if edge is not a loop
    if (_points[first] != _points[last]) {
        // compare signed distance to ray extending from first segment of edge if first primitive is closest
        edge_distance sdsqr_first0 = signed_primitive_distance_squared(p, first);
        edge_distance sdsqr_first = -signed_ray_distance_squared(_points[pfirst + 1], _points[pfirst], p);
        if (sdsqr_first0 == sdsqr_min && sdsqr_first < sdsqr_min) {
            sdsqr_min = sdsqr_first;
        }
        // compare signed distance from ray extending from last segment of edge if last primitive is closest
        edge_distance sdsqr_last0 = signed_primitive_distance_squared(p, last);
        edge_distance sdsqr_last = signed_ray_distance_squared(_points[plast - 1], _points[plast], p);
        if (sdsqr_last0 == sdsqr_min && sdsqr_last < sdsqr_min) {
            sdsqr_min = sdsqr_last;
        }
    }

    return std::copysign(std::sqrt(std::abs(sdsqr_min.signed_distance_sqr)), sdsqr_min.signed_distance_sqr);
}

//------------------------------------------------------------------------------
bool glyph::point_inside_glyph(vec2 p) const
{
    int n = 0;
    // Count the number of times a segment from the given point to an arbitrary
    // point outside the glyph crosses segments from the glyph. Since the glyph
    // can have overlapping contours it is necessary to track whether the line
    // crosses "into" or "out of" of the contour at each intersection. If the
    // sum is zero then the point is outside of the glyph.
    for (std::size_t ii = 0, sz = _edges.size(); ii < sz; ++ii) {
        for (std::size_t jj = _edges[ii].first; jj <= _edges[ii].last; ++jj) {
            std::size_t first = _primitives[jj].first;
            std::size_t last = _primitives[jj].last;
            switch (_primitives[jj].type) {
                case glyph::primitive_type::segment: {
                    n += signed_intersect_segments(p, vec2(-32.f, -16.f), _points[first], _points[last]);
                    break;
                }
                case glyph::primitive_type::qspline: {
                    n += signed_intersect_qspline(p, vec2(-32.f, -16.f), &_points[first], &_points[last]);
                    break;
                }
                case glyph::primitive_type::cspline:
                default:
                    break;
            }
        }
    }
    return n == 0;
}

//------------------------------------------------------------------------------
vec3 glyph::signed_edge_distance_channels(vec2 p) const
{
    edge_distance dsqr_min[3] = {{FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}};
    std::size_t emin[3] = {};

    if (!_edges.size()) {
        return vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    }

    // find closest edge for each channel
    for (std::size_t ii = 0, sz = _edges.size(); ii < sz; ++ii) {
        edge_distance dsqr = signed_edge_distance_squared(p, ii);
        for (std::size_t ch = 0; ch < 3; ++ch) {
            if ((_edge_channels[ii] & (1 << ch)) && dsqr < dsqr_min[ch]) {
                dsqr_min[ch] = dsqr;
                emin[ch] = ii;
            }
        }
    }

    return vec3(
        signed_edge_pseudo_distance(p, emin[0]),
        signed_edge_pseudo_distance(p, emin[1]),
        signed_edge_pseudo_distance(p, emin[2])
    );
}

//------------------------------------------------------------------------------
float glyph::signed_edge_distance(vec2 p) const
{
    edge_distance dsqr_min = {FLT_MAX, -FLT_MAX};
    std::size_t emin = {};

    if (!_edges.size()) {
        return FLT_MAX;
    }

    // find closest edge
    for (std::size_t ii = 0, sz = _edges.size(); ii < sz; ++ii) {
        edge_distance dsqr = signed_edge_distance_squared(p, ii);
        if (dsqr < dsqr_min) {
            dsqr_min = dsqr;
            emin = ii;
        }
    }

    return signed_edge_pseudo_distance(p, emin);
}

//------------------------------------------------------------------------------
std::size_t glyph::signed_edge_index(vec2 p) const
{
    edge_distance dsqr_min = {FLT_MAX, -FLT_MAX};
    std::size_t emin = {};

    // find closest edge
    for (std::size_t ii = 0, sz = _edges.size(); ii < sz; ++ii) {
        edge_distance dsqr = signed_edge_distance_squared(p, ii);
        if (dsqr < dsqr_min) {
            dsqr_min = dsqr;
            emin = ii;
        }
    }

    return emin;
}

} // namespace render
