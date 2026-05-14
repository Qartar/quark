// p_rigidbody.cpp
//

#include "p_rigidbody.h"
#include "p_shape.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
mat4 rigid_body::get_transform() const
{
    return mat4::transform(_position, mat3(_rotation));
}

//------------------------------------------------------------------------------
mat4 rigid_body::get_inverse_transform() const
{
    return mat4::inverse_transform(_position, mat3(_rotation));
}

//------------------------------------------------------------------------------
bounds3 rigid_body::get_bounds() const
{
    bounds3 b = bounds3(_shape->calculate_bounds(mat3_identity));
    return b.transform(get_transform());
}

//------------------------------------------------------------------------------
double rigid_body::get_kinetic_energy() const
{
    double energy = 0.0;

    if (_inverse_mass) {
        vec3 linear = _linear_velocity;
        energy += 0.5 * linear.dot(linear) / _inverse_mass;
    }
    if (_inverse_inertia) {
        vec3 angular = _angular_velocity;
        energy += 0.5 * dot(angular, angular) / _inverse_inertia;
    }

    return energy;
}

//------------------------------------------------------------------------------
void rigid_body::apply_impulse(vec3 impulse)
{
    assert(!isnan(impulse));
    _linear_velocity += impulse * _inverse_mass;
}

//------------------------------------------------------------------------------
void rigid_body::apply_impulse(vec3 impulse, vec3 position)
{
    assert(!isnan(impulse));
    _linear_velocity += impulse * _inverse_mass;
    _angular_velocity += cross(position - _position, impulse) * _inverse_inertia;
}

//------------------------------------------------------------------------------
void rigid_body::set_mass(double mass)
{
    if (mass && _inverse_mass) {
        double ratio = mass * _inverse_mass;
        _inverse_mass *= ratio;
        _inverse_inertia *= ratio;
    } else if (mass) {
        _inverse_mass = 1.0 / mass;
        _shape->calculate_mass_properties(_inverse_mass, _center_of_mass, _inverse_inertia);
    } else {
        _inverse_mass = 0.0;
        _inverse_inertia = 0.0;
    }
}

//------------------------------------------------------------------------------
bool rigid_body::contains_point(vec3 point) const
{
    return _shape->contains_point((point * get_inverse_transform()).to_vec2());
}

} // namespace physics
