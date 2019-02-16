// r_model.h
//

#pragma once

#include "cm_vector.h"
#include "cm_color.h"
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace render {

class system;

//------------------------------------------------------------------------------
class model
{
public:
    struct rect
    {
        vec2 center;
        vec2 size;
        float gamma;
    };

    template<std::size_t Size>
    model(rect const (&rects)[Size])
        : model(rects, Size)
    {}

    std::vector<vec2> const& vertices() const { return _vertices; }

    bounds bounds() const { return _bounds; }
    bool contains(vec2 point) const;

protected:
    friend system;

    std::vector<vec2> _vertices;
    std::vector<color3> _colors;
    std::vector<uint16_t> _indices;
    ::bounds _bounds;

protected:
    model(rect const* rects, std::size_t num_rects);
};

} // namespace render

extern render::model ship_model;
