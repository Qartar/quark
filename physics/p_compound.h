//  p_compound.h
//

#pragma once

#include "p_shape.h"

#include <memory>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
class compound_shape : public shape
{
public:
    struct child_shape {
        std::unique_ptr<physics::shape> shape;
        vec2 position;
        float rotation;
        mat3 transform;
        mat3 inverse_transform;
    };

public:
    template<std::size_t size> compound_shape(child_shape (&&children)[size]) {
        for (std::size_t ii = 0; ii < size; ++ii) {
            _children.push_back(std::move(children[ii]));
            _children.back().transform = mat3::transform(_children.back().position, _children.back().rotation);
            _children.back().inverse_transform = mat3::inverse_transform(_children.back().position, _children.back().rotation);
        }
    }

    virtual shape_type type() const override {
        return shape_type::compound;
    }

    virtual bool contains_point(vec2 point) const override;

    virtual vec2 supporting_vertex(vec2 direction) const override;

    virtual float calculate_area() const override;

    virtual void calculate_mass_properties(float inverse_mass, vec2& center_of_mass, float& inverse_inertia) const override;

    virtual bounds calculate_bounds(mat3 transform) const override;

    child_shape const* begin() const { return _children.data(); }

    child_shape const* end() const { return _children.data() + _children.size(); }

protected:
    std::vector<child_shape> _children;
};

} // namespace physics
