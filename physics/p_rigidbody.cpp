// p_rigidbody.cpp
//

#include "p_rigidbody.h"
#include "p_shape.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
double rigid_body::get_kinetic_energy() const
{
    double energy = 0.0;

    if (_inverse_mass) {
        vec2 linear = _motion.get_linear_velocity();
        energy += 0.5 * linear.dot(linear) / _inverse_mass;
    }
    if (_inverse_inertia) {
        double angular = _motion.get_angular_velocity();
        energy += 0.5 * angular * angular / _inverse_inertia;
    }

    return energy;
}

//------------------------------------------------------------------------------
void rigid_body::apply_impulse(vec2 impulse)
{
    assert(!isnan(impulse));
    vec2 linear = _motion.get_linear_velocity();
    linear += impulse * _inverse_mass;
    _motion.set_linear_velocity(linear);
}

//------------------------------------------------------------------------------
void rigid_body::apply_impulse(vec2 impulse, vec2 position)
{
    assert(!isnan(impulse));
    vec2 linear = _motion.get_linear_velocity();
    linear += impulse * _inverse_mass;
    _motion.set_linear_velocity(linear);

    double angular = _motion.get_angular_velocity();
    vec2 displacement = position - _motion.get_position();
    angular += displacement.cross(impulse) * _inverse_inertia;
    _motion.set_angular_velocity(angular);
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
        _motion.get_shape()->calculate_mass_properties(_inverse_mass, _center_of_mass, _inverse_inertia);
    } else {
        _inverse_mass = 0.0;
        _inverse_inertia = 0.0;
    }
}

} // namespace physics
