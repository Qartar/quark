//  p_compound.cpp
//

#include "p_compound.h"
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
bool compound_shape::contains_point(vec2 point) const
{
    for (auto& child : _children) {
        if (child.shape->contains_point(point * child.inverse_transform())) {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
vec2 compound_shape::supporting_vertex(vec2 /*direction*/) const
{
    assert(false);
    return vec2_zero;
}

//------------------------------------------------------------------------------
double compound_shape::calculate_area() const
{
    double area = 0.0;
    // Assume subshapes are not overlapping
    for (auto const& child : _children) {
        area += child.shape->calculate_area();
    }
    return area;
}

//------------------------------------------------------------------------------
void compound_shape::calculate_mass_properties(double inverse_mass, vec2& center_of_mass, double& inverse_inertia) const
{
    center_of_mass = vec2_zero;

    double inverse_area = 1.f / calculate_area();
    double inertia = 0.f;

    for (auto const& child : _children) {
        vec2 child_center_of_mass;
        double child_inverse_inertia;

        double child_area = child.shape->calculate_area();
        double child_mass = inverse_mass ? child_area * inverse_area / inverse_mass : 0.f;

        child.shape->calculate_mass_properties(
            inverse_mass,
            child_center_of_mass,
            child_inverse_inertia);

        center_of_mass += child_center_of_mass * child.transform() * child_mass;
        // by parallel axis theorem: I = I0 + md^2
        if (child_inverse_inertia) {
            inertia += 1.f / child_inverse_inertia + child_mass * child.position.length_sqr();
        }
    }

    center_of_mass *= inverse_mass;
    inverse_inertia = inertia ? 1.f / inertia : 0.f;
}

//------------------------------------------------------------------------------
bounds compound_shape::calculate_bounds(mat3 transform) const
{
    assert(_children.size());
    bounds out = _children[0].shape->calculate_bounds(_children[0].transform() * transform);
    for (std::size_t ii = 1, sz = _children.size(); ii < sz; ++ii) {
        out |= _children[ii].shape->calculate_bounds(_children[ii].transform() * transform);
    }
    return out;
}

} // namespace physics
