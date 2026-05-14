// p_trace.h
//

#pragma once

#include "cm_vector.h"
#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace physics {

class rigid_body;

//------------------------------------------------------------------------------
class trace
{
public:
    trace(rigid_body const* body, vec3 start, vec3 end);
    trace(rigid_body const* body_a, rigid_body const* body_b, double delta_time);

    double get_fraction() const { return _fraction; }

    contact const& get_contact() const {
        return _contact;
    }

protected:
    double _fraction;

    contact _contact;

    constexpr static int max_iterations = 64;
    constexpr static double epsilon = 1e-6;

protected:
    static double dispatch(contact& contact, motion motion_a, motion motion_b, double delta_time);
    static double compound_compound_dispatch(contact& contact, motion motion_a, motion motion_b, double delta_time);
    static double compound_convex_dispatch(contact& contact, motion motion_a, motion motion_b, double delta_time);
    static double convex_compound_dispatch(contact& contact, motion motion_a, motion motion_b, double delta_time);
    static double convex_convex_dispatch(contact& contact, motion motion_a, motion motion_b, double delta_time);
    static double convex_point_dispatch(contact& contact, motion motion_a, motion motion_b, double delta_time);
};

} // namespace physics
