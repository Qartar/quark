// p_rigidbody.h
//

#pragma once

#include "p_motion.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

class shape;
class material;

//------------------------------------------------------------------------------
class rigid_body
{
public:
    rigid_body(shape const* shape, material const* material, float mass)
        : _motion(shape)
        , _inverse_mass(0)
        , _inverse_inertia(0)
        , _center_of_mass(0,0)
        , _material(material)
    {
        set_mass(mass);
    }

    motion const& get_motion() const {
        return _motion;
    }

    //
    //  position
    //

    vec2 get_position() const {
        return _motion.get_position();
    }

    void set_position(vec2 position) {
        _motion.set_position(position);
    }

    float get_rotation() const {
        return _motion.get_rotation();
    }

    void set_rotation(float rotation) {
        _motion.set_rotation(rotation);
    }

    mat3 get_transform() const {
        return _motion.get_transform();
    }

    mat3 get_inverse_transform() const {
        return _motion.get_inverse_transform();
    }

    bounds get_bounds() const {
        return _motion.get_bounds();
    }

    //
    //  velocity
    //

    vec2 get_linear_velocity() const {
        return _motion.get_linear_velocity();
    }

    vec2 get_linear_velocity(vec2 position) const {
        return _motion.get_linear_velocity(position);
    }

    void set_linear_velocity(vec2 linear_velocity) {
        _motion.set_linear_velocity(linear_velocity);
    }

    float get_angular_velocity() const {
        return _motion.get_angular_velocity();
    }

    void set_angular_velocity(float angular_velocity) {
        _motion.set_angular_velocity(angular_velocity);
    }

    float get_kinetic_energy() const;

    //
    //  dynamics
    //

    void apply_impulse(vec2 impulse);

    void apply_impulse(vec2 impulse, vec2 position);

    //
    //  properties
    //

    float get_mass() const {
        return _inverse_mass ? 1.0f / _inverse_mass : 0.0f;
    }

    void set_mass(float mass);

    float get_inverse_mass() const {
        return _inverse_mass;
    }

    float get_inverse_inertia() const {
        return _inverse_inertia;
    }

    void set_shape(shape const* shape) {
        _motion.set_shape(shape);
    }

    shape const* get_shape() const {
        return _motion.get_shape();
    }

    void set_material(material const* material) {
        _material = material;
    }

    material const* get_material() const {
        return _material;
    }

protected:
    motion _motion;

    float _inverse_mass;
    float _inverse_inertia;
    vec2 _center_of_mass;

    material const* _material;
};

} // namespace physics
