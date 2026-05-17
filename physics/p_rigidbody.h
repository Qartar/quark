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
    rigid_body(shape const* shape, material const* material, double mass)
        : _shape(shape)
        , _position(vec3_zero)
        , _rotation(rot3_identity)
        , _linear_velocity(vec3_zero)
        , _angular_velocity(vec3_zero)
        , _inverse_mass(0)
        , _inverse_inertia(0)
        , _center_of_mass(0,0)
        , _material(material)
        , _handle_bits(0)
    {
        set_mass(mass);
    }

    //
    //  position
    //

    vec3 get_position() const {
        return _position;
    }

    void set_position(vec3 position) {
        _position = position;
    }

    rot3 get_rotation() const {
        return _rotation;
    }

    void set_rotation(rot3 rotation) {
        _rotation = rotation;
    }

    mat4 get_transform() const;

    mat4 get_inverse_transform() const;

    bounds3 get_bounds() const;
    bounds get_bounds(mat4 projection) const;

    //
    //  velocity
    //

    vec3 get_linear_velocity() const {
        return _linear_velocity;
    }

    vec3 get_linear_velocity(vec3 position) const {
        return _linear_velocity + cross(_angular_velocity, position - _position);
    }

    void set_linear_velocity(vec3 linear_velocity) {
        _linear_velocity = linear_velocity;
    }

    vec3 get_angular_velocity() const {
        return _angular_velocity;
    }

    void set_angular_velocity(vec3 angular_velocity) {
        _angular_velocity = angular_velocity;
    }

    double get_kinetic_energy() const;

    //
    //  dynamics
    //

    void apply_impulse(vec3 impulse);

    void apply_impulse(vec3 impulse, vec3 position);

    //
    //  properties
    //

    double get_mass() const {
        return _inverse_mass ? 1.0 / _inverse_mass : 0.0;
    }

    void set_mass(double mass);

    double get_inverse_mass() const {
        return _inverse_mass;
    }

    double get_inverse_inertia() const {
        return _inverse_inertia;
    }

    shape const* get_shape() const {
        return _shape;
    }

    material const* get_material() const {
        return _material;
    }

    //
    //  query
    //

    bool contains_point(vec3 point) const;

    //
    //  game interface
    //

    uint64_t get_handle_bits() const {
        return _handle_bits;
    }

    void set_handle_bits(uint64_t bits) {
        _handle_bits = bits;
    }

protected:
    shape const* _shape;

    vec3 _position;
    rot3 _rotation;
    vec3 _linear_velocity;
    vec3 _angular_velocity;

    bounds3 _bounds;

    double _inverse_mass;
    double _inverse_inertia;
    vec2 _center_of_mass;

    material const* _material;

    uint64_t _handle_bits;
};

} // namespace physics
