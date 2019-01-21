//  r_particle.h
//

#pragma once

#include "cm_time.h"
#include "cm_vector.h"
#include "cm_color.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
struct particle
{
    enum flag_bits
    {
        //! Fade alpha towards the particle center instead of outside
        invert = 1 << 0,

        //! Draw particle as a half-circle with an elliptical tail
        tail = 1 << 1,

        //! Draw particle as a ring, fade alpha away from leading edge
        ring = 1 << 2,
    };

    time_value time;
    float size, size_velocity;
    vec2 position, velocity, acceleration;
    float drag;
    color4 color, color_velocity;
    flag_bits flags;
};

} // namespace render
