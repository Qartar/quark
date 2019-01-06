// p_motion.cpp
//

#include "p_motion.h"
#include "p_shape.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
vec2 motion::get_linear_velocity(vec2 position) const
{
    // v = l + a x r
    //   = l - r x a
    return _linear_velocity - (position - _position).cross(_angular_velocity);
}

//------------------------------------------------------------------------------
mat3 motion::get_transform() const
{
    return mat3::transform(_position, _rotation);
}

//------------------------------------------------------------------------------
mat3 motion::get_inverse_transform() const
{
    return mat3::inverse_transform(_position, _rotation);
}

//------------------------------------------------------------------------------
bounds motion::get_bounds() const
{
    return _shape->calculate_bounds(get_transform());
}

} // namespace physics
