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

    vec2 mins() const { return _mins; }
    vec2 maxs() const { return _maxs; }

    bool contains(vec2 point) const;

protected:
    friend system;

    std::vector<vec2> _vertices;
    std::vector<color3> _colors;
    std::vector<uint16_t> _indices;
    vec2 _mins;
    vec2 _maxs;

protected:
    model(rect const* rects, std::size_t num_rects);
};

} // namespace render

extern render::model tank_body_model;
extern render::model tank_turret_model;

extern render::model ship_model;
