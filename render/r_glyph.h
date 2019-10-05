// r_glyph.h
//

#pragma once

#include "win_include.h"

#if !defined(_WIN32)
struct POINT {
    UINT x, y;
};
struct GLYPHMETRICS {
    UINT gmBlackBoxX;
    UINT gmBlackBoxY;
    UINT gmCellIncX;
    POINT gmptGlyphOrigin;
};
#endif // !defined(_WIN32)

////////////////////////////////////////////////////////////////////////////////
namespace render {

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
            || (std::abs(signed_distance_sqr) == std::abs(other.signed_distance_sqr)
                && orthogonal_distance > other.orthogonal_distance);
    }
    bool operator==(edge_distance const& other) const {
        return signed_distance_sqr == other.signed_distance_sqr
            && orthogonal_distance == other.orthogonal_distance;
    }
};

//------------------------------------------------------------------------------
class glyph
{
public:
    //! Generate glyph data from the current selected font on the device context
    static glyph from_hdc(HDC hdc, UINT ch);

    GLYPHMETRICS const& metrics() const { return _metrics; }
    bounds const& bounds() { return _bounds; }

    //! Returns the signed distance to the edge nearest to the given point
    float signed_edge_distance(vec2 point) const;
    //! Returns the signed distance by channel to the edges nearest to the given point
    vec3 signed_edge_distance_channels(vec2 point) const;
    //! Returns the index of the edge nearest to the given point
    std::size_t signed_edge_index(vec2 point) const;

    //! Returns true if the given point lies within the glyph contours
    bool point_inside_glyph(vec2 point) const;

    bool operator==(glyph const& other) const {
        return _outline.size() == other._outline.size()
            && !memcmp(_outline.data(), other._outline.data(), _outline.size())
            && !memcmp(&_metrics, &other._metrics, sizeof(_metrics));
    }

private:
    struct range {
        std::size_t first;
        std::size_t last;
    };

    enum class primitive_type {
        segment, //!< Line segment
        qspline, //!< Quadratic Bezier spline
        cspline, //!< Cubic Bezier spline
    };

    struct primitive {
        primitive_type type;
        std::size_t first; //!< Index of first point in the primitive
        std::size_t last; //!< Index of last point in the primitive
    };

private:
    //! All points on the glyph, including spline control points
    std::vector<vec2> _points;
    //! All primitives (i.e. segments and splines) of the glyph
    std::vector<primitive> _primitives;
    //! All edges, i.e. contiguous ranges of primitives
    std::vector<range> _edges;
    //! All contours, i.e. contiguous ranges of edges
    std::vector<range> _contours;

    //! Channel mask for each edge. All edges belong to at least two channels
    //! and all adjacent edges share one channel in common.
    std::vector<uint8_t> _edge_channels;

    //! Bounding box calculated from points
    ::bounds _bounds;

    //! Glyph metrics provided by GetGlyphOutlineW
    GLYPHMETRICS _metrics;
    //! Outline data provided by GetGlyphOutlineW
    std::vector<std::byte> _outline;

protected:
    //! Returns the signed distance to the given primitive from the given point.
    edge_distance signed_primitive_distance_squared(vec2 point, std::size_t primitive_index) const;
    //! Returns the signed distance to the given edge from the given point.
    edge_distance signed_edge_distance_squared(vec2 point, std::size_t edge_index) const;
    //! Returns the signed 'pseudo' distance to the edge from the given point.
    //! This treats edges as having rays extending on either end of the segment
    //! or spline in the same direction as the segment or tangent of the spline.
    float signed_edge_pseudo_distance(vec2 point, std::size_t edge_index) const;
};

} // namespace render
