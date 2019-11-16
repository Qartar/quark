// p_motion.h
//

#pragma once

#include "cm_bounds.h"
#include "cm_vector.h"
#include "cm_matrix.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

class shape;

//------------------------------------------------------------------------------
class motion
{
public:
    explicit motion(shape const* shape,
                    vec2 position = vec2_zero,
                    rot2 rotation = rot2_identity,
                    vec2 linear_velocity = vec2_zero,
                    float angular_velocity = 0)
        : _shape(shape)
        , _position(position)
        , _rotation(rotation)
        , _linear_velocity(linear_velocity)
        , _angular_velocity(angular_velocity)
    {}

    shape const* get_shape() const {
        return _shape;
    }

    //
    //  position
    //

    vec2 get_position() const {
        return _position;
    }

    void set_position(vec2 position) {
        _position = position;
    }

    rot2 get_rotation() const {
        return _rotation;
    }

    void set_rotation(rot2 rotation) {
        _rotation = rotation;
    }

    mat3 get_transform() const;

    mat3 get_inverse_transform() const;

    bounds get_bounds() const;

    //
    //  velocity
    //

    vec2 get_linear_velocity() const {
        return _linear_velocity;
    }

    vec2 get_linear_velocity(vec2 position) const;

    void set_linear_velocity(vec2 linear_velocity) {
        _linear_velocity = linear_velocity;
    }

    float get_angular_velocity() const {
        return _angular_velocity;
    }

    void set_angular_velocity(float angular_velocity) {
        _angular_velocity = angular_velocity;
    }

protected:
    shape const* _shape;

    vec2 _position;
    rot2 _rotation;

    vec2 _linear_velocity;
    float _angular_velocity;
};

} // namespace physics
