// p_trace.cpp
//

#include "p_trace.h"
#include "p_compound.h"
#include "p_rigidbody.h"
#include "p_shape.h"
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace physics {

//------------------------------------------------------------------------------
trace::trace(rigid_body const* body, vec2 start, vec2 end)
{
    physics::motion body_motion{
        body->get_shape(),
        body->get_position(),
        body->get_rotation()
    };

    physics::circle_shape shape(0);
    physics::motion point_motion{
        &shape,
        start,
        0,
        end - start
    };

    _fraction = dispatch(_contact, body_motion, point_motion, 1.f);
}

//------------------------------------------------------------------------------
trace::trace(rigid_body const* body_a, rigid_body const* body_b, float delta_time)
{
    _fraction = dispatch(_contact, body_a->get_motion(), body_b->get_motion(), delta_time);
}

//------------------------------------------------------------------------------
float trace::dispatch(contact& contact, motion motion_a, motion motion_b, float delta_time)
{
    auto fn = &convex_convex_dispatch;

    switch (motion_a.get_shape()->type()) {
        case shape_type::compound: {
            switch (motion_b.get_shape()->type()) {
                case shape_type::compound:
                    fn = &compound_compound_dispatch;
                    break;

                default:
                    fn = &compound_convex_dispatch;
                    break;
            }
            break;
        }

        default: {
            switch (motion_b.get_shape()->type()) {
                case shape_type::compound:
                    fn = &convex_compound_dispatch;
                    break;

                default:
                    fn = &convex_convex_dispatch;
                    break;
            }
            break;
        }
    }

    return fn(contact, motion_a, motion_b, delta_time);
}

//------------------------------------------------------------------------------
float trace::compound_compound_dispatch(contact& contact, motion motion_a, motion motion_b, float delta_time)
{
    assert(motion_a.get_shape()->type() == shape_type::compound);
    assert(motion_b.get_shape()->type() == shape_type::compound);

    physics::contact c;
    float f, fraction = 1.f;

    mat3 transform = motion_b.get_transform();

    for (auto& child : *static_cast<compound_shape const*>(motion_b.get_shape())) {
        motion child_motion{
            child.shape.get(),
            child.position * transform,
            child.rotation + motion_b.get_rotation(),
            motion_b.get_linear_velocity(),
            motion_b.get_angular_velocity()};

        if (child_motion.get_shape()->type() == shape_type::compound) {
            f = compound_compound_dispatch(c, motion_a, child_motion, delta_time);
        } else {
            f = compound_convex_dispatch(c, motion_a, child_motion, delta_time);
        }

        if (f < fraction) {
            contact = c;
            fraction = f;
        }
    }

    return fraction;
}

//------------------------------------------------------------------------------
float trace::compound_convex_dispatch(contact& contact, motion motion_a, motion motion_b, float delta_time)
{
    assert(motion_a.get_shape()->type() == shape_type::compound);
    assert(motion_b.get_shape()->type() != shape_type::compound);

    physics::contact c;
    float f, fraction = 1.f;

    mat3 transform = motion_a.get_transform();

    for (auto& child : *static_cast<compound_shape const*>(motion_a.get_shape())) {
        motion child_motion{
            child.shape.get(),
            child.position * transform,
            child.rotation + motion_a.get_rotation(),
            motion_a.get_linear_velocity(),
            motion_a.get_angular_velocity()};

        if (child_motion.get_shape()->type() == shape_type::compound) {
            f = convex_compound_dispatch(c, child_motion, motion_b, delta_time);
        } else {
            f = convex_convex_dispatch(c, child_motion, motion_b, delta_time);
        }

        if (f < fraction) {
            contact = c;
            fraction = f;
        }
    }

    return fraction;
}

//------------------------------------------------------------------------------
float trace::convex_compound_dispatch(contact& contact, motion motion_a, motion motion_b, float delta_time)
{
    assert(motion_a.get_shape()->type() != shape_type::compound);
    assert(motion_b.get_shape()->type() == shape_type::compound);

    float fraction = compound_convex_dispatch(contact, motion_b, motion_a, delta_time);
    contact.point += contact.normal * contact.distance;
    contact.normal *= -1.f;
    return fraction;
}

//------------------------------------------------------------------------------
float trace::convex_convex_dispatch(contact& contact, motion motion_a, motion motion_b, float delta_time)
{
    assert(motion_a.get_shape()->type() != shape_type::compound);
    assert(motion_b.get_shape()->type() != shape_type::compound);

    vec2 p0_a = motion_a.get_position();
    vec2 p0_b = motion_b.get_position();
    float r0_a = motion_a.get_rotation();
    float r0_b = motion_b.get_rotation();

    vec2 dp_a = motion_a.get_linear_velocity() * delta_time;
    vec2 dp_b = motion_b.get_linear_velocity() * delta_time;
    float dr_a = motion_a.get_angular_velocity() * delta_time;
    float dr_b = motion_b.get_angular_velocity() * delta_time;

    // todo: include rotation
    bounds bounds_a = bounds::from_translation(motion_a.get_bounds(), dp_a);
    bounds bounds_b = bounds::from_translation(motion_b.get_bounds(), dp_b);

    // bodies do not overlap during this time step
    if (!bounds_a.intersects(bounds_b)) {
        return 1.0f;
    }

    vec2 direction = dp_b - dp_a;
    float fraction = 0.f;

    if (direction == vec2_zero) {
        return 1.0f;
    }

    for (int num_iterations = 0; num_iterations < max_iterations && fraction < 1.0f; ++num_iterations) {
        motion_a.set_position(p0_a + dp_a * fraction);
        motion_a.set_rotation(r0_a + dr_a * fraction);
        motion_b.set_position(p0_b + dp_b * fraction);
        motion_b.set_rotation(r0_b + dr_b * fraction);

        contact = physics::collide(motion_a, motion_b).get_contact();

        direction = motion_b.get_linear_velocity(contact.point)
                  - motion_a.get_linear_velocity(contact.point);

        if (contact.normal.dot(direction) >= 0.0f) {
            return 1.f;
        }

        if (contact.distance < epsilon) {
            break;
        }

        fraction -= contact.distance / contact.normal.dot(direction);
        assert(!isnan(fraction));
    }

    assert(!isnan(fraction));
    return fraction > 1.0f ? 1.0f : fraction;
}

} // namespace physics
